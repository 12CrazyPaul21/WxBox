helpme = declare_plugin('helpme')

helpme.AUTO_SEND_TIMER_ID = 0

function helpme.timer_event(id)
    
end

function helpme.every_day_timer_event(id)
    wxbox.info('定时器消息')

    if (id == helpme.AUTO_SEND_TIMER_ID) then
        -- 发送消息
        wxbox.send_text_to_filehelper('你好文件传输助手')
        wxbox.info('已经发送')
    end
end

function helpme.start_auto_send_message()
    -- 时
    hour = 23
    -- 分
    minute = 40
    -- 秒
    second = 20

    helpme:kill_every_day_timer(helpme.AUTO_SEND_TIMER_ID)
    helpme:start_every_day_timer(helpme.AUTO_SEND_TIMER_ID, hour, minute, second)
    wxbox.info('已经设置每日定时发送')
end

function helpme.stop_auto_send_message()
    helpme:kill_every_day_timer(helpme.AUTO_SEND_TIMER_ID)
end

function helpme.handle_filehelper_text_message(event)
    message = event:message()

    if (message == '关机') then
        wxbox.shell('shutdown', '/s /t 120')
    elseif (message == '取消关机') then
        wxbox.shell('shutdown', '/a')
    elseif (message == '定时发送') then
        helpme.start_auto_send_message()
    elseif (message == '关闭定时发送') then
        helpme.stop_auto_send_message()
    elseif (string.find(message, '打开')) then
        for path in string.gmatch(message, ' ([^%s]+)') do
            wxbox.shell(path)
        end
    elseif (string.find(message, '提示')) then
        for msg in string.gmatch(message, ' ([^%s]+)') do
            wxbox.msgbox(msg)
        end
    else
        wxbox.speak(message)
    end
end

local text_message_handler = {
    ['filehelper'] = helpme.handle_filehelper_text_message
}

function helpme.receive_text_message(event)
    text_message_handler[event:wxid()](event)
end
