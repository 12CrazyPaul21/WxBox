-- declare a plugin, must be a global variable
chatgpt = declare_plugin('chatgpt')

-- common lib
local JSON = require('JSON')
local mime = require('mime')
local socket = require('socket')
local http = require('socket.http')
local urlencode = require('urlencode')

-- dynamic module
local ar = require('async_request')

function chatgpt.load()
    wxbox.info("chatgpt.load")
    wxbox.info('wxbox version : ' .. wxbox.version())
    wxbox.info('chatgpt module storage path : ' .. wxbox.package_storage_path(chatgpt.plugin_name))

    -- register async_request module response callback
    ar.register_response_callback(chatgpt.async_request_callback)
    return true
end

function chatgpt.prereload()
    wxbox.info("chatgpt.prereload")
end

function chatgpt.unload()
    wxbox.info("chatgpt.unload")
end

function chatgpt.receive_raw_message(event)
    -- event:filter_message()
    -- event:substitute_wxid('filehelper')
    -- event:substitute_message('你好')
    wxbox.info('chatgpt.receive_raw_message : ' .. event:message())
end

function chatgpt.receive_message(event)
    wxbox.info('chatgpt.receive_message : ' .. event:message())
end

function chatgpt.receive_text_message(event)
    local wxid = event:wxid()
    local message = event:message()

    local prompt_hint = 'prompt: '
    local prompt_with_max_tokens_pattern = '^prompt%[(%d+)%]: (.+)'
    local prompt_image_hint = 'prompt_image: '

    max_tokens, pattern = string.match(message, prompt_with_max_tokens_pattern)

    if max_tokens ~= nil and pattern ~= nil then
        chatgpt.async_ask(wxid, pattern, max_tokens)
        -- local response = chatgpt.ask(pattern, max_tokens)
        -- wxbox.send_text(wxid, response)

        -- wxbox.info(pattern)
        -- wxbox.info(max_tokens)
        -- wxbox.info(response)
    elseif string.sub(message, 1, string.len(prompt_hint)) == prompt_hint then
        local prompt = string.sub(message, string.len(prompt_hint))
        chatgpt.async_ask(wxid, prompt, 100)
        -- local response = chatgpt.ask(prompt, 100)
        -- wxbox.send_text(wxid, response)

        -- wxbox.info(prompt)
        -- wxbox.info(response)
    elseif string.sub(message, 1, string.len(prompt_image_hint)) == prompt_image_hint then
        local prompt = string.sub(message, string.len(prompt_image_hint))
        chatgpt.async_gen_image(wxid, prompt)
    end

    wxbox.info('chatgpt.receive_text_message wxid : ' .. event:wxid())
    wxbox.info('chatgpt.receive_text_message message : ' .. event:message())
end

function chatgpt.send_text_message(event)
    -- event:substitute_wxid('filehelper')
    -- event:substitute_message('test')
    -- event:substitute_chatroom_talker_wxid('filehelper')
    wxbox.info("chatgpt.send_text_message : " .. event:message())
end

function chatgpt.login_wechat_event(event)
    wxbox.info('login')
end

function chatgpt.logout_wechat_event(event)
    wxbox.info('logout')
end

function chatgpt.exit_wechat_event(event)
    wxbox.info('exit wechat')
end

function chatgpt.timer_event(id)
    wxbox.info('timer event, timer id : ' .. id)
    wxbox.send_text_to_filehelper('timer event, timer id : ' .. id)
end

function chatgpt.every_day_timer_event(id)
    wxbox.info('every day timer event, timer id : ' .. id)
    wxbox.send_text_to_filehelper('every day timer event, timer id : ' .. id)
end

function chatgpt.hello()
    wxbox.info("storage path : " .. chatgpt.storage_path)
    return 'hello'
end

function chatgpt.long_time_task_test()
    for i=1,100,1 do
        wxbox.info(i)
        wxbox.send_text_to_filehelper(i)
        wxbox.sleep(1000)
    end
end

function chatgpt.dispatch_message()
    local hostEvent = wxbox_host_event.create()
    hostEvent:set_type(wxbox_host_event.ReportHelp)
    wxbox.dispatch_host_event(hostEvent)
end

function chatgpt.parse_a_json(j)
    local obj = JSON:decode(j)
    for k, v in pairs(obj) do
        wxbox.info("key : " .. k .. ", value : " ..  v)
    end
end

function chatgpt.say_template_hello()
    wxbox.info(template.hello())
end

function chatgpt.test_timer(period)
    chatgpt:start_timer(0, period)
end

function chatgpt.stop_test_timer()
    chatgpt:kill_timer(0)
end

function chatgpt.test_every_day_timer(hour, minute, second)
    chatgpt:start_every_day_timer(0, hour, minute, second)
end

function chatgpt.stop_test_every_day_timer()
    chatgpt:kill_every_day_timer(0)
end

function chatgpt.socket_version()
    wxbox.info(socket._VERSION)
end

function chatgpt.ask(prompt, max_tokens)
    local encoded_prompt = urlencode.encode_url(prompt)
    local response = chatgpt.get_request(
        'http://127.0.0.1:5000/question?prompt=' .. encoded_prompt .. '&max_tokens=' .. max_tokens
    )
    return response
end

function chatgpt.get_request(url)
    local success, response = pcall(http.request, url)
    if success then
        wxbox.info(response)
        return response
    else
        return '调用失败 : ' .. response
    end
end

function chatgpt.urlencode(url)
    return urlencode.encode_url(url)
end

function chatgpt.async_request_callback(response, data)
    local context = JSON:decode(data)
    local response_json = JSON:decode(response)

    if response_json['result_text'] ~= nil then
        wxbox.send_text(context['wxid'], response_json['result_text'])
        wxbox.info('send text to : ' .. context['wxid'])
    elseif response_json['path'] ~= nil then
        wxbox.send_file(context['wxid'], response_json['path'])
        wxbox.info('send file to : ' .. context['wxid'])
    end
end

function chatgpt.async_ask(wxid, prompt, max_tokens)
    local encoded_prompt = urlencode.encode_url(prompt)
    local url = 'http://127.0.0.1:5000/question?prompt=' .. encoded_prompt .. '&max_tokens=' .. max_tokens
    local context = {['wxid']=wxid, ['url']=url}

    ar.request(url, JSON:encode(context))
end

function chatgpt.async_gen_image(wxid, prompt)
    local encoded_prompt = urlencode.encode_url(prompt)
    local url = 'http://127.0.0.1:5000/image?prompt=' .. encoded_prompt
    local context = {['wxid']=wxid, ['url']=url}

    ar.request(url, JSON:encode(context))
end