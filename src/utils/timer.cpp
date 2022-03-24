#include <utils/common.h>
#include <shared_mutex>

static std::unordered_map<time_t, wb_timer::TimerDesc> g_timerRecord;
static std::shared_mutex                               g_timerMutex;
static std::time_t                                     g_timerGlobalIndex     = 0;
static std::time_t                                     g_everyDayTimerCounter = 0;
static std::promise<void>                              g_everyDayTimerKillSign;

//
// wb_timer Inner
//

static bool Inner_IsTimerAlive(const std::string& nspace, int id, bool isPeriodTimer)
{
    return std::any_of(g_timerRecord.cbegin(), g_timerRecord.cend(), [&](const auto& p) {
        return !p.second.nspace.compare(nspace) && p.second.id == id && p.second.isPeriodTimer == isPeriodTimer;
    });
}

static std::time_t Inner_GetTimerGlobalId(const std::string& nspace, int id, bool isPeriodTimer)
{
    auto it = std::find_if(g_timerRecord.cbegin(), g_timerRecord.cend(), [&](const auto& p) {
        return !p.second.nspace.compare(nspace) && p.second.id == id && p.second.isPeriodTimer == isPeriodTimer;
    });

    if (it == g_timerRecord.end()) {
        return 0;
    }

    return it->first;
}

static std::time_t Inner_GenerateTimerGlobalId()
{
    std::time_t timerGlobalId = g_timerGlobalIndex + 1;
    if (!timerGlobalId) {
        ++timerGlobalId;
    }

    return g_timerRecord.find(timerGlobalId) == g_timerRecord.end() ? timerGlobalId : 0;
}

static void Inner_StopEveryDayTimer()
{
    try {
        g_everyDayTimerKillSign.set_value();
    }
    catch (...) {
    }
}

#if WXBOX_IN_WINDOWS_OS

static void APIENTRY Inner_PeriodTimerRoutine(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
    WXBOX_UNREF(dwTimerLowValue);
    WXBOX_UNREF(dwTimerHighValue);

    std::time_t timerGlobalId = (std::time_t)lpArgToCompletionRoutine;
    if (!timerGlobalId) {
        return;
    }

    std::shared_lock<std::shared_mutex> lock(g_timerMutex);
    if (g_timerRecord.find(timerGlobalId) == g_timerRecord.end()) {
        return;
    }

    wb_timer::TimerDesc& td = g_timerRecord.at(timerGlobalId);
    td.lastTriggerTimestamp = wb_process::GetCurrentTimestamp(false);

    if (td.callback) {
        td.callback(td.nspace, td.id, td.isPeriodTimer);
    }
}

static void APIENTRY Inner_EveryDayTimerRoutine(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
    WXBOX_UNREF(lpArgToCompletionRoutine);
    WXBOX_UNREF(dwTimerLowValue);
    WXBOX_UNREF(dwTimerHighValue);

    std::shared_lock<std::shared_mutex> lock(g_timerMutex);

    std::time_t timestamp = wb_process::GetCurrentTimestamp(false);

    for (auto& p : g_timerRecord) {
        if (p.second.isPeriodTimer) {
            continue;
        }

        auto delta = difftime(timestamp, p.second.lastTriggerTimestamp);
        if (delta > 0 && delta < 86400) {
            continue;
        }

        bool success = true;

        try {
            struct std::tm _triggerTm;
            if (!localtime_s(&_triggerTm, &timestamp)) {
                _triggerTm.tm_hour            = p.second.datePeriod.hour;
                _triggerTm.tm_min             = p.second.datePeriod.minute;
                _triggerTm.tm_sec             = p.second.datePeriod.second;
                p.second.lastTriggerTimestamp = mktime(&_triggerTm);
            }
            else {
                success = false;
            }
        }
        catch (...) {
            success = false;
        }

        if (!success) {
            p.second.lastTriggerTimestamp = timestamp;
        }

        wb_process::async_task([](wb_timer::TimerCallback callback, std::string nspace, int id, bool isPeriodTimer) {
            callback(nspace, id, isPeriodTimer);
        },
                               p.second.callback,
                               p.second.nspace,
                               p.second.id,
                               p.second.isPeriodTimer);
    }
}

static bool Inner_StartEveryDayTimer()
{
    std::promise<bool> readySign;

    try {
        g_everyDayTimerKillSign = std::promise<void>();
    }
    catch (...) {
    }

    std::thread([&]() {
        HANDLE hTimer = CreateWaitableTimerA(NULL, FALSE, NULL);
        if (!hTimer) {
            readySign.set_value(false);
            return;
        }

        // each second
        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = -1000 * 10000;

        if (!SetWaitableTimer(hTimer, &liDueTime, 1000, Inner_EveryDayTimerRoutine, nullptr, FALSE)) {
            readySign.set_value(false);
            CloseHandle(hTimer);
            return;
        }

        auto exitSignFuture = g_everyDayTimerKillSign.get_future();
        readySign.set_value(true);

        try {
            while (exitSignFuture.wait_for(std::chrono::milliseconds(10)) == std::future_status::timeout) {
                SleepEx(1, TRUE);
            }
        }
        catch (...) {
        }

        CancelWaitableTimer(hTimer);
        CloseHandle(hTimer);
    })
        .detach();

    bool result = false;

    try {
        result = readySign.get_future().get();
    }
    catch (...) {
    }

    return result;
}

static bool StartPeriodTimer_Windows(const std::string& nspace, int id, int period, wb_timer::TimerCallback callback)
{
    if (nspace.empty() || id < 0 || callback == nullptr) {
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(g_timerMutex);
    if (Inner_IsTimerAlive(nspace, id, true)) {
        return false;
    }

    std::time_t timerGlobalId = Inner_GenerateTimerGlobalId();
    if (!timerGlobalId) {
        return false;
    }

    wb_timer::TimerDesc timerDesc(nspace, id, callback);
    timerDesc.isPeriodTimer = true;
    timerDesc.period        = period;

    std::thread([&]() {
        HANDLE hTimer = CreateWaitableTimerA(NULL, FALSE, NULL);
        if (!hTimer) {
            timerDesc.readySign.set_value(false);
            return;
        }

        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = -timerDesc.period * 10000;

        if (!SetWaitableTimer(hTimer, &liDueTime, timerDesc.period, Inner_PeriodTimerRoutine, (LPVOID)timerGlobalId, FALSE)) {
            timerDesc.readySign.set_value(false);
            CloseHandle(hTimer);
            return;
        }

        auto exitSignFuture = timerDesc.killSign.get_future();
        timerDesc.readySign.set_value(true);

        try {
            while (exitSignFuture.wait_for(std::chrono::milliseconds(10)) == std::future_status::timeout) {
                SleepEx(1, TRUE);
            }
        }
        catch (...) {
        }

        CancelWaitableTimer(hTimer);
        CloseHandle(hTimer);
    })
        .detach();

    bool timerReady = false;

    try {
        timerReady = timerDesc.readySign.get_future().get();
    }
    catch (...) {
    }

    if (!timerReady) {
        return false;
    }

    g_timerGlobalIndex           = timerGlobalId;
    g_timerRecord[timerGlobalId] = std::move(timerDesc);
    return true;
}

static bool StartEveryDayPeriodTimer_Windows(const std::string& nspace, int id, const wb_timer::EveryDayPeriodDesc& periodDesc, wb_timer::TimerCallback callback)
{
    if (nspace.empty() || id < 0 || callback == nullptr) {
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(g_timerMutex);
    if (Inner_IsTimerAlive(nspace, id, false)) {
        return false;
    }

    std::time_t timerGlobalId = Inner_GenerateTimerGlobalId();
    if (!timerGlobalId) {
        return false;
    }

    wb_timer::TimerDesc timerDesc(nspace, id, callback);
    timerDesc.isPeriodTimer = false;
    timerDesc.datePeriod    = periodDesc;

    timerDesc.datePeriod.hour   = timerDesc.datePeriod.hour > 23 ? 23 : timerDesc.datePeriod.hour;
    timerDesc.datePeriod.minute = timerDesc.datePeriod.minute > 59 ? 59 : timerDesc.datePeriod.minute;
    timerDesc.datePeriod.second = timerDesc.datePeriod.second > 59 ? 59 : timerDesc.datePeriod.second;

    struct std::tm tm;
    std::time_t    lastTriggerTimestamp = wb_process::GetCurrentTimestamp(false);

    if (wb_process::GetCurrentDate(tm)) {
        struct std::tm lastTm;
        std::memcpy(&lastTm, &tm, sizeof(std::tm));

        lastTm.tm_hour = timerDesc.datePeriod.hour;
        lastTm.tm_min  = timerDesc.datePeriod.minute;
        lastTm.tm_sec  = timerDesc.datePeriod.second;

        try {
            std::time_t tm_ts     = mktime(&tm);
            std::time_t lastTm_ts = mktime(&lastTm);

            lastTriggerTimestamp = lastTm_ts;

            if (std::difftime(tm_ts, lastTm_ts) < 0) {
                lastTriggerTimestamp -= 86400;
            }
        }
        catch (...) {
        }
    }
    else {
        lastTriggerTimestamp -= 86400;
    }

    timerDesc.lastTriggerTimestamp = lastTriggerTimestamp;

    //
    // start timer
    //

    if (g_everyDayTimerCounter == 0) {
        if (!Inner_StartEveryDayTimer()) {
            return false;
        }
    }

    ++g_everyDayTimerCounter;

    g_timerGlobalIndex           = timerGlobalId;
    g_timerRecord[timerGlobalId] = std::move(timerDesc);

    return true;
}

#elif WXBOX_IN_MAC_OS

static bool StartPeriodTimer_Mac(const std::string& nspace, int id, int period, wb_timer::TimerCallback callback)
{
    throw std::exception("StartPeriodTimer_Mac stub");
    return false;
}

static bool StartEveryDayPeriodTimer_Mac(const std::string& nspace, int id, const wb_timer::EveryDayPeriodDesc& periodDesc, wb_timer::TimerCallback callback)
{
    throw std::exception("StartEveryDayPeriodTimer_Mac stub");
    return false;
}

#endif

//
// wb_timer
//

bool wxbox::util::timer::IsEveryDayPeriodTimer(const std::string& nspace, int id)
{
    std::shared_lock<std::shared_mutex> lock(g_timerMutex);
    return Inner_IsTimerAlive(nspace, id, false);
}

bool wxbox::util::timer::IsPeriodTimer(const std::string& nspace, int id)
{
    std::shared_lock<std::shared_mutex> lock(g_timerMutex);
    return Inner_IsTimerAlive(nspace, id, true);
}

bool wxbox::util::timer::StartPeriodTimer(const std::string& nspace, int id, int period, TimerCallback callback)
{
#if WXBOX_IN_WINDOWS_OS
    return StartPeriodTimer_Windows(nspace, id, period, callback);
#elif WXBOX_IN_MAC_OS
    return StartPeriodTimer_Mac(nspace, id, period, callback);
#endif
}

bool wxbox::util::timer::StartEveryDayPeriodTimer(const std::string& nspace, int id, const EveryDayPeriodDesc& periodDesc, TimerCallback callback)
{
#if WXBOX_IN_WINDOWS_OS
    return StartEveryDayPeriodTimer_Windows(nspace, id, periodDesc, callback);
#elif WXBOX_IN_MAC_OS
    return StartEveryDayPeriodTimer_Mac(nspace, id, periodDesc, callback);
#endif
}

void wxbox::util::timer::StopPeriodTimer(const std::string& nspace, int id)
{
    std::unique_lock<std::shared_mutex> lock(g_timerMutex);
    auto                                timerGlobalId = Inner_GetTimerGlobalId(nspace, id, true);
    if (!timerGlobalId) {
        return;
    }

    auto it = g_timerRecord.find(timerGlobalId);
    try {
        it->second.killSign.set_value();
    }
    catch (...) {
    }

    g_timerRecord.erase(it);
}

void wxbox::util::timer::StopPeriodTimerWithNameSpace(const std::string& nspace)
{
    std::unique_lock<std::shared_mutex> lock(g_timerMutex);

    for (auto it = g_timerRecord.begin(); it != g_timerRecord.end();) {
        if (!it->second.isPeriodTimer || it->second.nspace.compare(nspace)) {
            ++it;
            continue;
        }

        try {
            it->second.killSign.set_value();
        }
        catch (...) {
        }

        it = g_timerRecord.erase(it);
    }
}

void wxbox::util::timer::StopEveryDayPeriodTimer(const std::string& nspace, int id)
{
    std::unique_lock<std::shared_mutex> lock(g_timerMutex);
    auto                                timerGlobalId = Inner_GetTimerGlobalId(nspace, id, false);
    if (!timerGlobalId) {
        return;
    }

    auto it = g_timerRecord.find(timerGlobalId);
    g_timerRecord.erase(it);

    --g_everyDayTimerCounter;
    if (!g_everyDayTimerCounter) {
        Inner_StopEveryDayTimer();
    }
}

void wxbox::util::timer::StopEveryDayPeriodTimerWithNameSpace(const std::string& nspace)
{
    std::unique_lock<std::shared_mutex> lock(g_timerMutex);

    for (auto it = g_timerRecord.begin(); it != g_timerRecord.end();) {
        if (it->second.isPeriodTimer || it->second.nspace.compare(nspace)) {
            ++it;
            continue;
        }

        it = g_timerRecord.erase(it);
        --g_everyDayTimerCounter;
    }

    if (!g_everyDayTimerCounter) {
        Inner_StopEveryDayTimer();
    }
}

void wxbox::util::timer::StopAllTimer()
{
    std::unique_lock<std::shared_mutex> lock(g_timerMutex);

    if (g_everyDayTimerCounter) {
        Inner_StopEveryDayTimer();
        g_everyDayTimerCounter = 0;
    }
    g_timerGlobalIndex = 0;

    for (auto& td : g_timerRecord) {
        try {
            td.second.killSign.set_value();
        }
        catch (...) {
        }
    }

    g_timerRecord.clear();
}

void wxbox::util::timer::StopAllTimerWithNameSpace(const std::string& nspace)
{
    std::unique_lock<std::shared_mutex> lock(g_timerMutex);

    for (auto it = g_timerRecord.begin(); it != g_timerRecord.end();) {
        if (it->second.nspace.compare(nspace)) {
            ++it;
            continue;
        }

        if (it->second.isPeriodTimer) {
            try {
                it->second.killSign.set_value();
            }
            catch (...) {
            }
        }
        else {
            --g_everyDayTimerCounter;
        }

        it = g_timerRecord.erase(it);
    }

    if (!g_everyDayTimerCounter) {
        Inner_StopEveryDayTimer();
    }
}