-- declare a plugin, must be a global variable
example = declare_plugin('example')

-- common lib
local JSON = require('JSON')

-- dynamic module
local template = require('template')

function example.load()
    print("example.load")
    print('wxbox version : ' .. wxbox.version())
    print('example module storage path : ' .. wxbox.package_storage_path(example.plugin_name))
    return true
end

function example.prereload()
    print("example.prereload")
end

function example.unload()
    print("example.unload")
end

function example.receive_raw_message(event)
    -- event:filter_message()
    -- event:substitute_wxid('filehelper')
    -- event:substitute_message('test')
    -- wxbox.info('receive_raw_message : ' .. event:message())
end

function example.receive_message(event)
    wxbox.info('receive_message : ' .. event:message())
end

function example.receive_text_message(event)
    print("example.receive_text_message")
    print("    wxid : " .. event:wxid())
    print("    message : " .. event:message())
    wxbox.info('example.receive_text_message : ' .. event:message())
end

function example.send_text_message(event)
    print("example.send_text_message")
    print("    wxid : " .. event:wxid())
    print("    message : " .. event:message())
    -- wxbox.info("example.send_text_message : " .. event:message())
    -- event:substitute_wxid('filehelper')
    -- event:substitute_chatroom_talker_wxid('filehelper')
    -- event:substitute_message('test')
end

function example.login_wechat_event(event)
    wxbox.info('login')
end

function example.logout_wechat_event(event)
    wxbox.info('logout')
end

function example.exit_wechat_event(event)
    wxbox.info('exit')
end

function example.hello()
    print("hello im example module")
    print("storage path : " .. example.storage_path)
    return 'hello'
end

function example.test()
    -- wxbox.info(wxbox.WeChatMessageType.PLAINTEXT)
    -- wxbox.info(wxbox.WeChatMessageType.PICTURE)
    -- wxbox.info(wxbox.WeChatMessageType.AUDIO)
    -- wxbox.info(wxbox.WeChatMessageType.VIDEO)
    -- wxbox.info(wxbox.WeChatMessageType.EMOJI)
    -- wxbox.info(wxbox.WeChatMessageType.FILE)
    -- wxbox.info(wxbox.WeChatMessageType.WAKE_CONTACT_DIALOG)
    -- wxbox.info(wxbox.WeChatMessageType.REVOKE_MESSAGE)

    -- contact = wxbox.get_contact_with_wxnumber('ITXXXL')
    -- if (contact) then
    --     return contact.nickname
    -- end

    -- contacts = wxbox.get_all_contacts()
    -- return contacts[1].nickname
end

function example.dispatch_message()
    local hostEvent = wxbox_host_event.create()
    hostEvent:set_type(wxbox_host_event.SendTextMesage)
    hostEvent:set_wxid('filehelper')
    hostEvent:set_text_message('hello im example module')
    wxbox.dispatch_host_event(hostEvent)
end

function example.parse_a_json(j)
    local obj = JSON:decode(j)
    for k, v in pairs(obj) do
        print(k, v)
    end
end

function example.say_template_hello()
    print(template.hello())
end