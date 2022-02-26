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
    wxbox.info('receive_raw_message : ' .. event:message())
end

function example.receive_message(event)
    wxbox.info('receive_message : ' .. event:wxid())
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
    wxbox.info("example.send_text_message : " .. event:message())
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