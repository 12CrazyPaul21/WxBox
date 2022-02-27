#include <plugin/plugin.h>

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

static int __wxbox_package_storage_path(lua_State* L)
{
    const char* moduleName = luaL_checkstring(L, 1);
    luaL_argcheck(L, moduleName != nullptr, 1, "module name is required");

    lua_pushstring(L, wb_file::JoinPath(wb_plugin::GetPluginVirtualMachineStorageRoot(), moduleName).c_str());
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
    wb_plugin_wechat::FetchProfile(profile);
    lua_pushstring(L, profile.wxid.c_str());
    return 1;
}

static int __wxbox_profile_wxnumber(lua_State* L)
{
    wb_wx::WeChatProfile profile;
    wb_plugin_wechat::FetchProfile(profile);
    lua_pushstring(L, profile.wxnumber.c_str());
    return 1;
}

static int __wxbox_profile_nickname(lua_State* L)
{
    wb_wx::WeChatProfile profile;
    wb_plugin_wechat::FetchProfile(profile);
    lua_pushstring(L, profile.nickname.c_str());
    return 1;
}

static int __wxbox_nickname_to_wxid(lua_State* L)
{
    const char* nickname = luaL_checkstring(L, 1);
    luaL_argcheck(L, nickname != nullptr, 1, "nickname is required");

    wb_wx::WeChatContact contact;
    if (!wb_plugin_wechat::GetContactWithNickName(nickname, contact)) {
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

    std::string wxnumber = wb_plugin_wechat::WxidToWxNumber(wxid);
    lua_pushstring(L, wxnumber.c_str());
    return 1;
}

static int __wxbox_wxnumber_to_wxid(lua_State* L)
{
    const char* wxnumber = luaL_checkstring(L, 1);
    luaL_argcheck(L, wxnumber != nullptr, 1, "wxnumber is required");

    std::string wxid = wb_plugin_wechat::WxNumberToWxid(wxnumber);
    lua_pushstring(L, wxid.c_str());
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
    if (!wb_plugin_wechat::GetContactWithWxid(wxid, contact)) {
        return 0;
    }

    return __wxbox_profile_inner_push_contact(L, contact) ? 1 : 0;
}

static int __wxbox_profile_get_contact_with_wxnumber(lua_State* L)
{
    const char* wxnumber = luaL_checkstring(L, 1);
    luaL_argcheck(L, wxnumber != nullptr, 1, "wxnumber is required");

    wb_wx::WeChatContact contact;
    if (!wb_plugin_wechat::GetContactWithWxNumber(wxnumber, contact)) {
        return 0;
    }

    return __wxbox_profile_inner_push_contact(L, contact) ? 1 : 0;
}

static int __wxbox_profile_get_all_contacts(lua_State* L)
{
    std::vector<wb_wx::WeChatContact> contacts;
    if (!wb_plugin_wechat::GetAllContacts(contacts)) {
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
static int __wxbox_profile_send_text_to_filehelper(lua_State* L)
{
    const char* message = lua_tostring(L, 1);
    luaL_argcheck(L, message != nullptr, 1, "invalid message");

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
    event->sendMessageArgs->chatroom    = false;
    event->sendMessageArgs->useWxNumber = false;
    event->sendMessageArgs->wxid        = "filehelper";
    event->sendMessageArgs->message     = message;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.send_picture_to_filehelper <image path>
static int __wxbox_profile_send_picture_to_filehelper(lua_State* L)
{
    const char* imgPath = lua_tostring(L, 1);
    luaL_argcheck(L, imgPath != nullptr, 1, "invalid image path");

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

    event->sendMessageArgs->messageType = (uint32_t)wb_wx::WeChatMessageType::PICTURE;
    event->sendMessageArgs->chatroom    = false;
    event->sendMessageArgs->useWxNumber = false;
    event->sendMessageArgs->wxid        = "filehelper";
    event->sendMessageArgs->imgPath     = imgPath;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.send_file_to_filehelper <file path>
static int __wxbox_profile_send_file_to_filehelper(lua_State* L)
{
    const char* filePath = lua_tostring(L, 1);
    luaL_argcheck(L, filePath != nullptr, 1, "invalid file path");

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

    event->sendMessageArgs->messageType = (uint32_t)wb_wx::WeChatMessageType::FILE;
    event->sendMessageArgs->chatroom    = false;
    event->sendMessageArgs->useWxNumber = false;
    event->sendMessageArgs->wxid        = "filehelper";
    event->sendMessageArgs->filePath    = filePath;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.send_text <wxid>, <text message>
static int __wxbox_profile_send_text(lua_State* L)
{
    const char* wxid = lua_tostring(L, 1);
    luaL_argcheck(L, wxid != nullptr, 1, "invalid wxid");

    const char* message = lua_tostring(L, 2);
    luaL_argcheck(L, message != nullptr, 2, "invalid message");

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
    event->sendMessageArgs->chatroom    = false;
    event->sendMessageArgs->useWxNumber = false;
    event->sendMessageArgs->wxid        = wxid;
    event->sendMessageArgs->message     = message;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.send_picture <wxid>, <image path>
static int __wxbox_profile_send_picture(lua_State* L)
{
    const char* wxid = lua_tostring(L, 1);
    luaL_argcheck(L, wxid != nullptr, 1, "invalid wxid");

    const char* imgPath = lua_tostring(L, 2);
    luaL_argcheck(L, imgPath != nullptr, 2, "invalid image path");

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

    event->sendMessageArgs->messageType = (uint32_t)wb_wx::WeChatMessageType::PICTURE;
    event->sendMessageArgs->chatroom    = false;
    event->sendMessageArgs->useWxNumber = false;
    event->sendMessageArgs->wxid        = wxid;
    event->sendMessageArgs->imgPath     = imgPath;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.send_file <wxid>, <file path>
static int __wxbox_profile_send_file(lua_State* L)
{
    const char* wxid = lua_tostring(L, 1);
    luaL_argcheck(L, wxid != nullptr, 1, "invalid wxid");

    const char* filePath = lua_tostring(L, 2);
    luaL_argcheck(L, filePath != nullptr, 2, "invalid file path");

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

    event->sendMessageArgs->messageType = (uint32_t)wb_wx::WeChatMessageType::FILE;
    event->sendMessageArgs->chatroom    = false;
    event->sendMessageArgs->useWxNumber = false;
    event->sendMessageArgs->wxid        = wxid;
    event->sendMessageArgs->filePath    = filePath;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.send_text_with_wxnumber <wxnumber>, <text message>
static int __wxbox_profile_send_text_with_wxnumber(lua_State* L)
{
    const char* wxnumber = lua_tostring(L, 1);
    luaL_argcheck(L, wxnumber != nullptr, 1, "invalid wxnumber");

    const char* message = lua_tostring(L, 2);
    luaL_argcheck(L, message != nullptr, 2, "invalid message");

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
    event->sendMessageArgs->chatroom    = false;
    event->sendMessageArgs->useWxNumber = true;
    event->sendMessageArgs->wxnumber    = wxnumber;
    event->sendMessageArgs->message     = message;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.send_picture_with_wxnumber <wxnumber>, <image path>
static int __wxbox_profile_send_picture_with_wxnumber(lua_State* L)
{
    const char* wxnumber = lua_tostring(L, 1);
    luaL_argcheck(L, wxnumber != nullptr, 1, "invalid wxnumber");

    const char* imgPath = lua_tostring(L, 2);
    luaL_argcheck(L, imgPath != nullptr, 2, "invalid image path");

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

    event->sendMessageArgs->messageType = (uint32_t)wb_wx::WeChatMessageType::PICTURE;
    event->sendMessageArgs->chatroom    = false;
    event->sendMessageArgs->useWxNumber = true;
    event->sendMessageArgs->wxnumber    = wxnumber;
    event->sendMessageArgs->imgPath     = imgPath;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.send_file_with_wxnumber <wxnumber>, <file path>
static int __wxbox_profile_send_file_with_wxnumber(lua_State* L)
{
    const char* wxnumber = lua_tostring(L, 1);
    luaL_argcheck(L, wxnumber != nullptr, 1, "invalid wxnumber");

    const char* filePath = lua_tostring(L, 2);
    luaL_argcheck(L, filePath != nullptr, 2, "invalid file path");

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

    event->sendMessageArgs->messageType = (uint32_t)wb_wx::WeChatMessageType::FILE;
    event->sendMessageArgs->chatroom    = false;
    event->sendMessageArgs->useWxNumber = true;
    event->sendMessageArgs->wxnumber    = wxnumber;
    event->sendMessageArgs->filePath    = filePath;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.send_text_to_chatroom <roomWxid>, <text message>, <optional notify list...>
static int __wxbox_profile_send_text_to_chatroom(lua_State* L)
{
    const char* roomWxid = lua_tostring(L, 1);
    luaL_argcheck(L, roomWxid != nullptr, 1, "invalid roomWxid");

    const char* message = lua_tostring(L, 2);
    luaL_argcheck(L, message != nullptr, 2, "invalid message");

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
    event->sendMessageArgs->chatroom    = true;
    event->sendMessageArgs->useWxNumber = false;
    event->sendMessageArgs->wxid        = roomWxid;
    event->sendMessageArgs->message     = message;

    // fill notify list
    for (int i = 3; i <= lua_gettop(L); i++) {
        const char* notifyWxid = lua_tostring(L, i);
        luaL_argcheck(L, notifyWxid != nullptr, i, "invalid notify wxid");
        event->sendMessageArgs->notifyWxidLists.push_back(notifyWxid);
    }

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.send_picture_to_chatroom <roomWxid>, <image path>
static int __wxbox_profile_send_picture_to_chatroom(lua_State* L)
{
    const char* roomWxid = lua_tostring(L, 1);
    luaL_argcheck(L, roomWxid != nullptr, 1, "invalid roomWxid");

    const char* imgPath = lua_tostring(L, 2);
    luaL_argcheck(L, imgPath != nullptr, 2, "invalid image path");

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

    event->sendMessageArgs->messageType = (uint32_t)wb_wx::WeChatMessageType::PICTURE;
    event->sendMessageArgs->chatroom    = true;
    event->sendMessageArgs->useWxNumber = false;
    event->sendMessageArgs->wxid        = roomWxid;
    event->sendMessageArgs->imgPath     = imgPath;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.send_file_to_chatroom <roomWxid>, <file path>
static int __wxbox_profile_send_file_to_chatroom(lua_State* L)
{
    const char* roomWxid = lua_tostring(L, 1);
    luaL_argcheck(L, roomWxid != nullptr, 1, "invalid roomWxid");

    const char* filePath = lua_tostring(L, 2);
    luaL_argcheck(L, filePath != nullptr, 2, "invalid file path");

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

    event->sendMessageArgs->messageType = (uint32_t)wb_wx::WeChatMessageType::FILE;
    event->sendMessageArgs->chatroom    = true;
    event->sendMessageArgs->useWxNumber = false;
    event->sendMessageArgs->wxid        = roomWxid;
    event->sendMessageArgs->filePath    = filePath;

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.chatroom_notify <roomWxid>, <optional notify list...>
static int __wxbox_profile_chatroom_notify(lua_State* L)
{
    const char* roomWxid = lua_tostring(L, 1);
    luaL_argcheck(L, roomWxid != nullptr, 1, "invalid roomWxid");

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
    event->sendMessageArgs->chatroom    = true;
    event->sendMessageArgs->useWxNumber = false;
    event->sendMessageArgs->wxid        = roomWxid;
    event->sendMessageArgs->message     = "";

    // fill notify list
    for (int i = 2; i <= lua_gettop(L); i++) {
        const char* notifyWxid = lua_tostring(L, i);
        luaL_argcheck(L, notifyWxid != nullptr, i, "invalid notify wxid");
        event->sendMessageArgs->notifyWxidLists.push_back(notifyWxid);
    }

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

// usage : >>wxbox.chatroom_notify_all <roomWxid>
static int __wxbox_profile_chatroom_notify_all(lua_State* L)
{
    const char* roomWxid = lua_tostring(L, 1);
    luaL_argcheck(L, roomWxid != nullptr, 1, "invalid roomWxid");

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
    event->sendMessageArgs->chatroom    = true;
    event->sendMessageArgs->useWxNumber = false;
    event->sendMessageArgs->wxid        = roomWxid;
    event->sendMessageArgs->message     = "";
    event->sendMessageArgs->notifyWxidLists.push_back("notify@all");

    wb_plugin::DispatchPluginToHostEvent(std::move(event));
    lua_pushboolean(L, true);
    return 1;
}

//
// export module
//

const struct luaL_Reg wxbox::plugin::internal::WxBoxModuleMethods[] = {
    {"version", __wxbox_version},
    {"package_storage_path", __wxbox_package_storage_path},
    {"dispatch_host_event", __wxbox_dispatch_host_event},
    {"info", __wxbox_info},
    {"warning", __wxbox_warning},
    {"error", __wxbox_error},
    {"clear", __wxbox_clear},
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

    {"send_text_to_filehelper", __wxbox_profile_send_text_to_filehelper},
    {"send_picture_to_filehelper", __wxbox_profile_send_picture_to_filehelper},
    {"send_file_to_filehelper", __wxbox_profile_send_file_to_filehelper},
    {"download", __wxbox_profile_send_file_to_filehelper},

    {"send_text", __wxbox_profile_send_text},
    {"send_picture", __wxbox_profile_send_picture},
    {"send_file", __wxbox_profile_send_file},

    {"send_text_with_wxnumber", __wxbox_profile_send_text_with_wxnumber},
    {"send_picture_with_wxnumber", __wxbox_profile_send_picture_with_wxnumber},
    {"send_file_with_wxnumber", __wxbox_profile_send_file_with_wxnumber},

    {"send_text_to_chatroom", __wxbox_profile_send_text_to_chatroom},
    {"send_picture_to_chatroom", __wxbox_profile_send_picture_to_chatroom},
    {"send_file_to_chatroom", __wxbox_profile_send_file_to_chatroom},

    {"chatroom_notify", __wxbox_profile_chatroom_notify},
    {"chatroom_notify_all", __wxbox_profile_chatroom_notify_all},

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