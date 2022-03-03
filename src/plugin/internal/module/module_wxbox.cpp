#include <plugin/plugin.h>
#include <random>

//
// inner helper
//

static inline std::string __inner_generate_temp_file_name(const std::string& prefix)
{
    static std::default_random_engine              re;
    static std::uniform_int_distribution<uint16_t> ud(0, 1000);

    std::stringstream ss;
    ss << prefix << "_" << wb_process::TimeStampToDate<std::chrono::milliseconds>(wb_process::GetCurrentTimestamp(), false) << ud(re);
    return ss.str();
}

static int __inner_send_text(lua_State* L, const std::string& target, bool isChatroom, bool isWxNumber, const std::string& message, std::vector<std::string>&& notifyWxidLists)
{
    auto event = wb_plugin::BuildHostEventModel();
    if (!event) {
        lua_pushboolean(L, false);
        return 1;
    }

    event->type            = wb_plugin::HostEventType::SendMesage;
    event->sendMessageArgs = std::make_shared<wb_plugin::PluginSendWeChatMessage>();
    if (!event->sendMessageArgs) {
        lua_pushboolean(L, false);
        return 1;
    }

    event->sendMessageArgs->messageType = (uint32_t)wb_wx::WeChatMessageType::PLAINTEXT;
    event->sendMessageArgs->chatroom    = isChatroom;
    event->sendMessageArgs->useWxNumber = isWxNumber;
    if (isWxNumber) {
        event->sendMessageArgs->wxnumber = target;
    }
    else {
        event->sendMessageArgs->wxid = target;
    }
    event->sendMessageArgs->message         = message;
    event->sendMessageArgs->notifyWxidLists = std::move(notifyWxidLists);

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

static int __inner_check_file_valid(lua_State* L, const std::string& filePath)
{
    auto nativeFilePath = wb_string::Utf8ToNativeString(filePath);

    // check whether file exists
    if (!wb_file::IsPathExists(nativeFilePath)) {
        lua_pushstring(L, "file is not exists");
        return 1;
    }

    //
    // check file size, max 1G Bytes
    //

    auto filesize = wb_file::GetFileSize(nativeFilePath);

    if (!filesize) {
        lua_pushstring(L, "file is empty");
        return 1;
    }
    else if (filesize > 1073741824) {
        lua_pushstring(L, "file size no more than 1GB");
        return 1;
    }

    return 0;
}

static int __inner_send_file(lua_State* L, const std::string& target, bool isChatroom, bool isWxNumber, const std::string& filePath, bool isPicture)
{
    // check file valid, if invalid return error message
    if (__inner_check_file_valid(L, filePath)) {
        return 1;
    }

    auto event = wb_plugin::BuildHostEventModel();
    if (!event) {
        lua_pushboolean(L, false);
        return 1;
    }

    event->type            = wb_plugin::HostEventType::SendMesage;
    event->sendMessageArgs = std::make_shared<wb_plugin::PluginSendWeChatMessage>();
    if (!event->sendMessageArgs) {
        lua_pushboolean(L, false);
        return 1;
    }

    event->sendMessageArgs->chatroom = isChatroom;

    if (isPicture) {
        event->sendMessageArgs->messageType = (uint32_t)wb_wx::WeChatMessageType::PICTURE;
        event->sendMessageArgs->imgPath     = filePath;
    }
    else {
        event->sendMessageArgs->messageType = (uint32_t)wb_wx::WeChatMessageType::FILE;
        event->sendMessageArgs->filePath    = filePath;
    }

    event->sendMessageArgs->useWxNumber = isWxNumber;

    if (isWxNumber) {
        event->sendMessageArgs->wxnumber = target;
    }
    else {
        event->sendMessageArgs->wxid = target;
    }

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

//
// wxbox module methods
//

static int __wxbox_version(lua_State* L)
{
    std::stringstream ss;
    ss << "wxbot-v" << WXBOT_VERSION;

    lua_pushstring(L, ss.str().c_str());
    return 1;
}

static int __wxbox_help(lua_State* L)
{
    WXBOX_UNREF(L);

    auto event = wb_plugin::BuildHostEventModel();
    if (!event) {
        return 0;
    }

    event->type = wb_plugin::HostEventType::ReportHelp;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    return 0;
}

static int __wxbox_generate_temp_file_name(lua_State* L)
{
    const char* prefix = luaL_checkstring(L, 1);
    luaL_argcheck(L, prefix != nullptr, 1, "prefix is required");

    lua_pushstring(L, __inner_generate_temp_file_name(prefix).c_str());
    return 1;
}

static int __wxbox_global_temp_folder_path(lua_State* L)
{
    lua_pushstring(L, wb_string::NativeToUtf8String(wb_plugin::GetPluginVirtualMachineGlobalTempRoot()).c_str());
    return 1;
}

static int __wxbox_package_storage_path(lua_State* L)
{
    const char* moduleName = luaL_checkstring(L, 1);
    luaL_argcheck(L, moduleName != nullptr, 1, "module name is required");

    lua_pushstring(L, wb_file::JoinPath(wb_string::NativeToUtf8String(wb_plugin::GetPluginVirtualMachineStorageRoot()), moduleName).c_str());
    return 1;
}

static int __wxbox_dispatch_host_event(lua_State* L)
{
    auto ptr = wb_plugin::FetchUserDataPointer<wb_plugin::HostEventModel, HOST_EVENT_MODEL_NAME>(L);
    luaL_argcheck(L, ptr != nullptr, 1, "null userdata");

    auto duplicate = wb_plugin::CopyHostEventModel(ptr);
    if (!duplicate) {
        return 0;
    }

    wb_plugin::DispatchPluginToHostEvent(duplicate);
    return 0;
}

static int __wxbox_dispatch_log(lua_State* L, wb_plugin::PluginLogLevel level)
{
    const char* message = lua_tostring(L, 1);
    luaL_argcheck(L, message != nullptr, 1, "invalid log message");

    auto event = wb_plugin::BuildHostEventModel();
    if (!event) {
        return 0;
    }

    event->type = wb_plugin::HostEventType::Log;
    event->log  = std::make_shared<wb_plugin::PluginLog>();
    if (!event->log) {
        return 0;
    }

    event->log->level   = level;
    event->log->message = message;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    return 0;
}

static int __wxbox_snap(lua_State* L)
{
    auto snapFileName = wb_file::JoinPath(wb_plugin::GetPluginVirtualMachineGlobalTempRoot(), __inner_generate_temp_file_name("snap") + ".png");
    if (!wb_platform::CaptureMonitorSnap(snapFileName)) {
        lua_pushboolean(L, false);
        return 1;
    }

    return __inner_send_file(L, "filehelper", false, false, wb_string::NativeToUtf8String(snapFileName), true);
}

static int __wxbox_snap_main_monitor(lua_State* L)
{
    auto snapFileName = wb_file::JoinPath(wb_plugin::GetPluginVirtualMachineGlobalTempRoot(), __inner_generate_temp_file_name("snap_main") + ".png");
    if (!wb_platform::CaptureMainMonitorSnap(snapFileName)) {
        lua_pushboolean(L, false);
        return 1;
    }

    return __inner_send_file(L, "filehelper", false, false, wb_string::NativeToUtf8String(snapFileName), true);
}

static int __wxbox_info(lua_State* L)
{
    return __wxbox_dispatch_log(L, wb_plugin::PluginLogLevel::Information);
}

static int __wxbox_warning(lua_State* L)
{
    return __wxbox_dispatch_log(L, wb_plugin::PluginLogLevel::Warning);
}

static int __wxbox_error(lua_State* L)
{
    return __wxbox_dispatch_log(L, wb_plugin::PluginLogLevel::Error);
}

static int __wxbox_clear(lua_State* L)
{
    WXBOX_UNREF(L);

    auto event = wb_plugin::BuildHostEventModel();
    if (!event) {
        return 0;
    }

    event->type = wb_plugin::HostEventType::ClearCommandResultScreen;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    return 0;
}

// >>wxbox.sleep: <millisecond>
static int __wxbox_sleep(lua_State* L)
{
    if (lua_gettop(L) != 1) {
        return 0;
    }

    lua_Integer ms = luaL_checkinteger(L, 1);
    luaL_argcheck(L, ms > 0 && ms <= 3000, 1, "sleep time must be greater than zero and less than 3000 millisecond");

    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return 0;
}

// >>wxbox.shell: <command>, [args...]
static int __wxbox_shell(lua_State* L)
{
    const char* command = luaL_checkstring(L, 1);
    luaL_argcheck(L, command != nullptr, 1, "command invalid");

    std::vector<std::string> args;

    // args list
    for (int i = 2; i <= lua_gettop(L); i++) {
        const char* arg = lua_tostring(L, i);
        luaL_argcheck(L, arg != nullptr, i, "invalid args");
        args.push_back(arg);
    }

    lua_pushboolean(L, wb_platform::Shell(command, args));
    return 1;
}

// >>wxbox.msgbox: <message>, [optional title]
static int __wxbox_msgbox(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    luaL_argcheck(L, message != nullptr, 1, "message invalid");

    std::string title;
    if (lua_gettop(L) > 1) {
        title = luaL_checkstring(L, 2);
    }

#if WXBOX_IN_WINDOWS_OS
    std::stringstream ss;
    ss << R"(vbscript:CreateObject("WScript.Shell").Popup(")" << message << R"(",0,")" << title << R"(",64)(window.close))";
    wb_platform::Shell("mshta", {wb_string::Utf8ToNativeString(ss.str())});
#else

#endif
    return 0;
}

// >>wxbox.speak: <message>
static int __wxbox_speak(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    luaL_argcheck(L, message != nullptr, 1, "message invalid");

#if WXBOX_IN_WINDOWS_OS
    std::stringstream ss;
    ss << R"(vbscript:CreateObject("SAPI.SpVoice").Speak(")" << wb_string::Utf8ToNativeString(message) << R"(")(window.close))";
    wb_platform::Shell("mshta", {ss.str()});
#else

#endif
    return 0;
}

static int __wxbox_lock_screen(lua_State* L)
{
    wb_platform::LockScreen();
    lua_pushboolean(L, true);
    return 1;
}

static int __wxbox_list_drives(lua_State* L)
{
    auto drives = wb_file::GetAllDrives();
    if (drives.empty()) {
        return 0;
    }

    std::stringstream ss;
    for (auto drive : drives) {
        ss << drive << std::endl;
    }

    lua_pushstring(L, ss.str().c_str());
    return 1;
}

static int __wxbox_list_files(lua_State* L)
{
    const char* dirPath = luaL_checkstring(L, 1);
    luaL_argcheck(L, dirPath != nullptr, 1, "dirpath is required");

    std::string nativeDirPath = wb_string::Utf8ToNativeString(dirPath);
    luaL_argcheck(L, wb_file::IsDirectory(nativeDirPath), 1, "invalid dirpath");

    auto files = wb_file::ListAllFiles(nativeDirPath);
    if (files.empty()) {
        lua_pushstring(L, "[ empty ]");
        return 1;
    }

    std::stringstream ss;
    for (auto file : files) {
        if (wb_file::IsDirectory(wb_file::JoinPath(nativeDirPath, file))) {
            ss << "./";
        }
        ss << wb_string::NativeToUtf8String(file) << std::endl;
    }

    lua_pushstring(L, ss.str().c_str());
    return 1;
}

/**
 * avoid_revoke
 * enable_raw_message_hook
 * enable_send_text_message_hook
 */
static int __wxbox_set_config(lua_State* L)
{
    if (lua_gettop(L) != 2) {
        return 0;
    }

    const bool  enabled    = lua_toboolean(L, 2);
    const char* configName = lua_tostring(L, 1);
    luaL_argcheck(L, configName != nullptr, 1, "invalid config");

    auto event = wb_plugin::BuildHostEventModel();
    if (!event) {
        return 0;
    }

    event->type         = wb_plugin::HostEventType::ChangeConfig;
    event->changeConfig = std::make_shared<wb_plugin::PluginChangeConfigMessage>();
    if (!event->changeConfig) {
        return 0;
    }

    event->changeConfig->configName = configName;
    event->changeConfig->enabled    = enabled;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    return 0;
}

static int __wxbox_uninject_wxbot(lua_State* L)
{
    WXBOX_UNREF(L);

    auto event = wb_plugin::BuildHostEventModel();
    if (!event) {
        return 0;
    }

    event->type = wb_plugin::HostEventType::UnInject;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    return 0;
}

static int __wxbox_exit_wxbox(lua_State* L)
{
    WXBOX_UNREF(L);

    auto event = wb_plugin::BuildHostEventModel();
    if (!event) {
        return 0;
    }

    event->type = wb_plugin::HostEventType::ExitWxBox;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    return 0;
}

static int __wxbox_is_logined(lua_State* L)
{
    lua_pushboolean(L, wb_crack::IsLoign());
    return 1;
}

static int __wxbox_logout(lua_State* L)
{
    WXBOX_UNREF(L);

    auto event = wb_plugin::BuildHostEventModel();
    if (!event) {
        return 0;
    }

    event->type = wb_plugin::HostEventType::Logout;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    return 0;
}

static int __wxbox_profile_wxid(lua_State* L)
{
    wb_wx::WeChatProfile profile;
    wb_crack::FetchProfile(profile);
    lua_pushstring(L, profile.wxid.c_str());
    return 1;
}

static int __wxbox_profile_wxnumber(lua_State* L)
{
    wb_wx::WeChatProfile profile;
    wb_crack::FetchProfile(profile);
    lua_pushstring(L, profile.wxnumber.c_str());
    return 1;
}

static int __wxbox_profile_nickname(lua_State* L)
{
    wb_wx::WeChatProfile profile;
    wb_crack::FetchProfile(profile);
    lua_pushstring(L, profile.nickname.c_str());
    return 1;
}

static int __wxbox_nickname_to_wxid(lua_State* L)
{
    const char* nickname = luaL_checkstring(L, 1);
    luaL_argcheck(L, nickname != nullptr, 1, "nickname is required");

    wb_wx::WeChatContact contact;
    if (!wb_crack::GetContactWithNickName(nickname, contact)) {
        lua_pushstring(L, "");
        return 1;
    }

    lua_pushstring(L, contact.wxid.c_str());
    return 1;
}

static int __wxbox_wxid_to_wxnumber(lua_State* L)
{
    const char* wxid = luaL_checkstring(L, 1);
    luaL_argcheck(L, wxid != nullptr, 1, "wxid is required");

    wb_wx::WeChatContact contact;
    if (!wb_crack::GetContactWithWxid(wxid, contact)) {
        lua_pushstring(L, "");
        return 1;
    }

    lua_pushstring(L, contact.wxnumber.c_str());
    return 1;
}

static int __wxbox_wxnumber_to_wxid(lua_State* L)
{
    const char* wxnumber = luaL_checkstring(L, 1);
    luaL_argcheck(L, wxnumber != nullptr, 1, "wxnumber is required");

    wb_wx::WeChatContact contact;
    if (!wb_crack::GetContactWithWxNumber(wxnumber, contact)) {
        lua_pushstring(L, "");
        return 1;
    }

    lua_pushstring(L, contact.wxid.c_str());
    return 1;
}

static bool __wxbox_profile_inner_push_contact(lua_State* L, const wb_wx::WeChatContact& contact)
{
    lua_newtable(L);

    lua_pushboolean(L, contact.chatroom);
    lua_setfield(L, -2, "chatroom");

    lua_pushstring(L, contact.nickname.c_str());
    lua_setfield(L, -2, "nickname");

    lua_pushstring(L, contact.wxnumber.c_str());
    lua_setfield(L, -2, "wxnumber");

    lua_pushstring(L, contact.wxid.c_str());
    lua_setfield(L, -2, "wxid");

    lua_pushstring(L, contact.remark.c_str());
    lua_setfield(L, -2, "remark");

    return true;
}

static int __wxbox_profile_get_contact_with_wxid(lua_State* L)
{
    const char* wxid = luaL_checkstring(L, 1);
    luaL_argcheck(L, wxid != nullptr, 1, "wxid is required");

    wb_wx::WeChatContact contact;
    if (!wb_crack::GetContactWithWxid(wxid, contact)) {
        return 0;
    }

    return __wxbox_profile_inner_push_contact(L, contact) ? 1 : 0;
}

static int __wxbox_profile_get_contact_with_wxnumber(lua_State* L)
{
    const char* wxnumber = luaL_checkstring(L, 1);
    luaL_argcheck(L, wxnumber != nullptr, 1, "wxnumber is required");

    wb_wx::WeChatContact contact;
    if (!wb_crack::GetContactWithWxNumber(wxnumber, contact)) {
        return 0;
    }

    return __wxbox_profile_inner_push_contact(L, contact) ? 1 : 0;
}

static int __wxbox_profile_get_all_contacts(lua_State* L)
{
    std::vector<wb_wx::WeChatContact> contacts;
    if (!wb_crack::CollectAllContact(contacts)) {
        return 0;
    }

    if (contacts.empty()) {
        return 0;
    }

    // build contacts table
    lua_newtable(L);

    for (size_t i = 0; i < contacts.size(); i++) {
        __wxbox_profile_inner_push_contact(L, contacts.at(i));
        lua_rawseti(L, -2, i);
    }

    return 1;
}

// usage : >>wxbox.send_text_to_filehelper <text message>
static int __wxbox_send_text_to_filehelper(lua_State* L)
{
    const char* message = lua_tostring(L, 1);
    luaL_argcheck(L, message != nullptr, 1, "invalid message");

    return __inner_send_text(L, "filehelper", false, false, message, {});
}

// usage : >>wxbox.send_picture_to_filehelper <image path>
static int __wxbox_send_picture_to_filehelper(lua_State* L)
{
    const char* imgPath = lua_tostring(L, 1);
    luaL_argcheck(L, imgPath != nullptr, 1, "invalid image path");

    return __inner_send_file(L, "filehelper", false, false, imgPath, true);
}

// usage : >>wxbox.send_file_to_filehelper <file path>
static int __wxbox_send_file_to_filehelper(lua_State* L)
{
    const char* filePath = lua_tostring(L, 1);
    luaL_argcheck(L, filePath != nullptr, 1, "invalid file path");

    return __inner_send_file(L, "filehelper", false, false, filePath, false);
}

// usage : >>wxbox.send_text <wxid>, <text message>
static int __wxbox_send_text(lua_State* L)
{
    const char* wxid = lua_tostring(L, 1);
    luaL_argcheck(L, wxid != nullptr, 1, "invalid wxid");

    const char* message = lua_tostring(L, 2);
    luaL_argcheck(L, message != nullptr, 2, "invalid message");

    return __inner_send_text(L, wxid, false, false, message, {});
}

// usage : >>wxbox.send_picture <wxid>, <image path>
static int __wxbox_send_picture(lua_State* L)
{
    const char* wxid = lua_tostring(L, 1);
    luaL_argcheck(L, wxid != nullptr, 1, "invalid wxid");

    const char* imgPath = lua_tostring(L, 2);
    luaL_argcheck(L, imgPath != nullptr, 2, "invalid image path");

    return __inner_send_file(L, wxid, false, false, imgPath, true);
}

// usage : >>wxbox.send_file <wxid>, <file path>
static int __wxbox_send_file(lua_State* L)
{
    const char* wxid = lua_tostring(L, 1);
    luaL_argcheck(L, wxid != nullptr, 1, "invalid wxid");

    const char* filePath = lua_tostring(L, 2);
    luaL_argcheck(L, filePath != nullptr, 2, "invalid file path");

    return __inner_send_file(L, wxid, false, false, filePath, false);
}

// usage : >>wxbox.send_text_with_wxnumber <wxnumber>, <text message>
static int __wxbox_send_text_with_wxnumber(lua_State* L)
{
    const char* wxnumber = lua_tostring(L, 1);
    luaL_argcheck(L, wxnumber != nullptr, 1, "invalid wxnumber");

    const char* message = lua_tostring(L, 2);
    luaL_argcheck(L, message != nullptr, 2, "invalid message");

    return __inner_send_text(L, wxnumber, false, true, message, {});
}

// usage : >>wxbox.send_picture_with_wxnumber <wxnumber>, <image path>
static int __wxbox_send_picture_with_wxnumber(lua_State* L)
{
    const char* wxnumber = lua_tostring(L, 1);
    luaL_argcheck(L, wxnumber != nullptr, 1, "invalid wxnumber");

    const char* imgPath = lua_tostring(L, 2);
    luaL_argcheck(L, imgPath != nullptr, 2, "invalid image path");

    return __inner_send_file(L, wxnumber, false, true, imgPath, true);
}

// usage : >>wxbox.send_file_with_wxnumber <wxnumber>, <file path>
static int __wxbox_send_file_with_wxnumber(lua_State* L)
{
    const char* wxnumber = lua_tostring(L, 1);
    luaL_argcheck(L, wxnumber != nullptr, 1, "invalid wxnumber");

    const char* filePath = lua_tostring(L, 2);
    luaL_argcheck(L, filePath != nullptr, 2, "invalid file path");

    return __inner_send_file(L, wxnumber, false, true, filePath, false);
}

// usage : >>wxbox.send_text_to_chatroom <roomWxid>, <text message>, [optional notify list...]
static int __wxbox_send_text_to_chatroom(lua_State* L)
{
    const char* roomWxid = lua_tostring(L, 1);
    luaL_argcheck(L, roomWxid != nullptr, 1, "invalid roomWxid");

    const char* message = lua_tostring(L, 2);
    luaL_argcheck(L, message != nullptr, 2, "invalid message");

    // fill notify list
    std::vector<std::string> notifyWxidLists;
    for (int i = 3; i <= lua_gettop(L); i++) {
        const char* notifyWxid = lua_tostring(L, i);
        luaL_argcheck(L, notifyWxid != nullptr, i, "invalid notify wxid");
        notifyWxidLists.push_back(notifyWxid);
    }

    return __inner_send_text(L, roomWxid, true, false, message, std::move(notifyWxidLists));
}

// usage : >>wxbox.send_picture_to_chatroom <roomWxid>, <image path>
static int __wxbox_send_picture_to_chatroom(lua_State* L)
{
    const char* roomWxid = lua_tostring(L, 1);
    luaL_argcheck(L, roomWxid != nullptr, 1, "invalid roomWxid");

    const char* imgPath = lua_tostring(L, 2);
    luaL_argcheck(L, imgPath != nullptr, 2, "invalid image path");

    return __inner_send_file(L, roomWxid, true, false, imgPath, true);
}

// usage : >>wxbox.send_file_to_chatroom <roomWxid>, <file path>
static int __wxbox_send_file_to_chatroom(lua_State* L)
{
    const char* roomWxid = lua_tostring(L, 1);
    luaL_argcheck(L, roomWxid != nullptr, 1, "invalid roomWxid");

    const char* filePath = lua_tostring(L, 2);
    luaL_argcheck(L, filePath != nullptr, 2, "invalid file path");

    return __inner_send_file(L, roomWxid, true, false, filePath, false);
}

// usage : >>wxbox.chatroom_notify <roomWxid>, [optional notify list...]
static int __wxbox_chatroom_notify(lua_State* L)
{
    const char* roomWxid = lua_tostring(L, 1);
    luaL_argcheck(L, roomWxid != nullptr, 1, "invalid roomWxid");

    // fill notify list
    std::vector<std::string> notifyWxidLists;
    for (int i = 2; i <= lua_gettop(L); i++) {
        const char* notifyWxid = lua_tostring(L, i);
        luaL_argcheck(L, notifyWxid != nullptr, i, "invalid notify wxid");
        notifyWxidLists.push_back(notifyWxid);
    }

    return __inner_send_text(L, roomWxid, true, false, "", std::move(notifyWxidLists));
}

// usage : >>wxbox.chatroom_notify_all <roomWxid>
static int __wxbox_chatroom_notify_all(lua_State* L)
{
    const char* roomWxid = lua_tostring(L, 1);
    luaL_argcheck(L, roomWxid != nullptr, 1, "invalid roomWxid");

    return __inner_send_text(L, roomWxid, true, false, "", {"notify@all"});
}

//
// export module
//

const struct luaL_Reg wxbox::plugin::internal::WxBoxModuleMethods[] = {
    {"version", __wxbox_version},
    {"help", __wxbox_help},
    {"generate_temp_file_name", __wxbox_generate_temp_file_name},
    {"global_temp_folder_path", __wxbox_global_temp_folder_path},
    {"package_storage_path", __wxbox_package_storage_path},
    {"dispatch_host_event", __wxbox_dispatch_host_event},

    {"snap", __wxbox_snap},
    {"snap_main_monitor", __wxbox_snap_main_monitor},

    {"info", __wxbox_info},
    {"warning", __wxbox_warning},
    {"error", __wxbox_error},
    {"clear", __wxbox_clear},

    {"sleep", __wxbox_sleep},
    {"shell", __wxbox_shell},
    {"msgbox", __wxbox_msgbox},
    {"speak", __wxbox_speak},
    {"lock_screen", __wxbox_lock_screen},
    {"list_drives", __wxbox_list_drives},
    {"list_files", __wxbox_list_files},

    {"set_config", __wxbox_set_config},
    {"uninject_wxbot", __wxbox_uninject_wxbot},
    {"exit_wxbox", __wxbox_exit_wxbox},

    {"is_logined", __wxbox_is_logined},
    {"logout", __wxbox_logout},
    {"profile_wxid", __wxbox_profile_wxid},
    {"profile_wxnumber", __wxbox_profile_wxnumber},
    {"profile_nickname", __wxbox_profile_nickname},

    {"nickname_to_wxid", __wxbox_nickname_to_wxid},
    {"wxid_to_wxnumber", __wxbox_wxid_to_wxnumber},
    {"wxnumber_to_wxid", __wxbox_wxnumber_to_wxid},
    {"chatroom_wxid", __wxbox_nickname_to_wxid},
    {"get_contact_with_wxid", __wxbox_profile_get_contact_with_wxid},
    {"get_contact_with_wxnumber", __wxbox_profile_get_contact_with_wxnumber},
    {"get_all_contacts", __wxbox_profile_get_all_contacts},

    {"send_text_to_filehelper", __wxbox_send_text_to_filehelper},
    {"send_picture_to_filehelper", __wxbox_send_picture_to_filehelper},
    {"send_file_to_filehelper", __wxbox_send_file_to_filehelper},
    {"download", __wxbox_send_file_to_filehelper},

    {"send_text", __wxbox_send_text},
    {"send_picture", __wxbox_send_picture},
    {"send_file", __wxbox_send_file},

    {"send_text_with_wxnumber", __wxbox_send_text_with_wxnumber},
    {"send_picture_with_wxnumber", __wxbox_send_picture_with_wxnumber},
    {"send_file_with_wxnumber", __wxbox_send_file_with_wxnumber},

    {"send_text_to_chatroom", __wxbox_send_text_to_chatroom},
    {"send_picture_to_chatroom", __wxbox_send_picture_to_chatroom},
    {"send_file_to_chatroom", __wxbox_send_file_to_chatroom},

    {"chatroom_notify", __wxbox_chatroom_notify},
    {"chatroom_notify_all", __wxbox_chatroom_notify_all},

    {NULL, NULL},
};

int wxbox::plugin::internal::__luaopen_wxbox_module(lua_State* L)
{
    luaL_newlib(L, WxBoxModuleMethods);

    //
    // build wechat message type enum
    //

    lua_newtable(L);

    lua_pushinteger(L, 0x01);
    lua_setfield(L, -2, "PLAINTEXT");
    lua_pushinteger(L, 0x03);
    lua_setfield(L, -2, "PICTURE");
    lua_pushinteger(L, 0x22);
    lua_setfield(L, -2, "AUDIO");
    lua_pushinteger(L, 0x2B);
    lua_setfield(L, -2, "VIDEO");
    lua_pushinteger(L, 0x2F);
    lua_setfield(L, -2, "EMOJI");
    lua_pushinteger(L, 0x31);
    lua_setfield(L, -2, "FILE");
    lua_pushinteger(L, 0x33);
    lua_setfield(L, -2, "WAKE_CONTACT_DIALOG");
    lua_pushinteger(L, 0x2712);
    lua_setfield(L, -2, "REVOKE_MESSAGE");

    // record WeChatMessageType in wxbox module
    lua_setfield(L, -2, "WeChatMessageType");

    return 1;
}

bool wxbox::plugin::internal::IsWxBoxModuleMethod(const std::string& methodName)
{
    for (const luaL_Reg* method = WxBoxModuleMethods; method->func; method++) {
        if (!methodName.compare(method->name)) {
            return true;
        }
    }
    return false;
}