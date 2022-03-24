#ifndef __WXBOX_UTILS_TIMER_H
#define __WXBOX_UTILS_TIMER_H

namespace wxbox {
    namespace util {
        namespace timer {

            //
            // Typedef
            //

            using TimerCallback = std::function<void(const std::string nspace, int id, bool isPeriodTimer)>;

            typedef struct _EveryDayPeriodDesc
            {
                uint8_t hour;
                uint8_t minute;
                uint8_t second;

                _EveryDayPeriodDesc(uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0)
                  : hour(hour)
                  , minute(minute)
                  , second(second)
                {
                }
            } EveryDayPeriodDesc, *PEveryDayPeriodDesc;

            typedef struct _TimerDesc
            {
                std::string        nspace;
                int                id;
                TimerCallback      callback;
                bool               isPeriodTimer;
                int                period;
                EveryDayPeriodDesc datePeriod;
                time_t             lastTriggerTimestamp;
                std::promise<bool> readySign;
                std::promise<void> killSign;

                _TimerDesc()
                {
                }

                explicit _TimerDesc(const std::string& nspace, int id, TimerCallback callback)
                  : nspace(nspace)
                  , id(id)
                  , callback(callback)
                  , isPeriodTimer(true)
                  , period(0)
                  , datePeriod()
                  , lastTriggerTimestamp(0)
                  , readySign()
                  , killSign()
                {
                }
            } TimerDesc, *PTimerDesc;

            //
            // Function
            //

            bool IsEveryDayPeriodTimer(const std::string& nspace, int id);
            bool IsPeriodTimer(const std::string& nspace, int id);

            bool StartPeriodTimer(const std::string& nspace, int id, int period, TimerCallback callback);
            bool StartEveryDayPeriodTimer(const std::string& nspace, int id, const EveryDayPeriodDesc& periodDesc, TimerCallback callback);

            void StopPeriodTimer(const std::string& nspace, int id);
            void StopPeriodTimerWithNameSpace(const std::string& nspace);

            void StopEveryDayPeriodTimer(const std::string& nspace, int id);
            void StopEveryDayPeriodTimerWithNameSpace(const std::string& nspace);

            void StopAllTimer();
            void StopAllTimerWithNameSpace(const std::string& nspace);
        }
    }
}

#endif  // #ifndef __WXBOX_UTILS_TIMER_H