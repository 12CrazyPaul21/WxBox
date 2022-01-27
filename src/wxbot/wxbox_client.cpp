#include <wxbox_client.hpp>

//
// WxBoxEndPoint
//

void wxbot::WxBoxEndPoint::OnDone(const Status& status)
{
    WXBOX_UNREF(status);

    if (!client) {
        return;
    }

    client->HandleWxBoxEndPointFinish();
}

void wxbot::WxBoxEndPoint::OnReadDone(bool ok)
{
    if (!ok) {
        Finish();
        return;
    }

    bool failed = !client || !client->HandleWxBoxServerRequest(fromServerPacket);
    fromServerPacket.Clear();

    if (failed) {
        Finish();
        return;
    }

    StartRead(&fromServerPacket);
}

//
// WxBoxClient
//

void wxbot::WxBoxClient::ResetChannel()
{
    channel = grpc::CreateChannel(serverURI, grpc::InsecureChannelCredentials());
    stub    = WxBox::NewStub(channel);
}

bool wxbot::WxBoxClient::Start()
{
    bool alreadyRunning = false;
    running.compare_exchange_strong(alreadyRunning, true);

    if (alreadyRunning) {
        return false;
    }

    // reset environment
    ClearMessageQueue();
    exitSignal = std::promise<void>();
    doneSignal = std::promise<void>();
    exitFuture = exitSignal.get_future();
    doneFuture = doneSignal.get_future();
    endpoint   = nullptr;

    worker = std::thread(std::bind(&wxbot::WxBoxClient::Routine, this));
    worker.detach();
    return true;
}

void wxbot::WxBoxClient::Stop()
{
    if (!running.load()) {
        return;
    }

    if (IsTimeToExit()) {
        return;
    }

    if (endpoint) {
        endpoint->Finish();
    }

    exitSignal.set_value();
    cv.notify_one();
}

void wxbot::WxBoxClient::Wait()
{
    if (!running.load()) {
        return;
    }

    doneFuture.wait();
}

void wxbot::WxBoxClient::PushMessage(WxBotMessage message)
{
    if (!running.load()) {
        return;
    }

    {
        std::unique_lock<std::shared_mutex> lock(rwmutex);
        queue.emplace_back(std::move(message));
    }
    cv.notify_one();
}

void wxbot::WxBoxClient::PushMessageAsync(WxBotMessage message)
{
    wb_process::async_task(std::bind(&WxBoxClient::PushMessage, this, std::placeholders::_1), message);
}

void wxbot::WxBoxClient::ClearMessageQueue()
{
    std::unique_lock<std::shared_mutex> lock(rwmutex);
    queue.clear();
}

void wxbot::WxBoxClient::PutMessageToWxBot(const wxbot::WxBotMessage& message)
{
    if (wxbotCallback) {
        wb_process::async_task(wxbotCallback, message);
    }
}

bool wxbot::WxBoxClient::SendPacket(const WxBotControlPacket& packet)
{
    if (!endpoint || endpoint->finished) {
        return false;
    }

    return endpoint->SendPacket(packet);
}

void wxbot::WxBoxClient::ChangeStatus(WxBoxClientStatus status)
{
    wxbot::WxBotMessage message           = wxbot::WxBotMessage(MsgRole::WxBot, WxBotMessageType::WxBoxClientStatusChange);
    message.u.wxBoxClientStatus.oldStatus = clientStatus;
    message.u.wxBoxClientStatus.newStatus = status;
    clientStatus                          = status;
    PutMessageToWxBot(message);
}

bool wxbot::WxBoxClient::TryConnectWxBoxServer()
{
    if (!Ping()) {
        return false;
    }

    endpoint = std::make_unique<wxbot::WxBoxEndPoint>(stub.get(), this);
    return endpoint->Handshake();
}

void wxbot::WxBoxClient::RetryConnect()
{
    wxbot::WxBotMessage msg(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotRetryConnect);
    PushMessage(std::move(msg));
}

bool wxbot::WxBoxClient::IsTimeToExit()
{
    return exitFuture.wait_for(std::chrono::milliseconds(1)) != std::future_status::timeout;
}

void wxbot::WxBoxClient::HandleWxBoxEndPointFinish()
{
    PushMessage(wxbot::WxBotMessage(wxbot::MsgRole::WxBot, wxbot::WxBotMessageType::WxBotConnectionLost));
}

bool wxbot::WxBoxClient::HandleWxBoxServerRequest(WxBoxControlPacket packet)
{
    if (!endpoint) {
        return false;
    }

    auto type = packet.type();

    switch (type) {
        case wxbox::ControlPacketType::HANDSHAKE_RESPONSE:
            if (packet.mutable_handshakeresponse()->success()) {
                endpoint->HandshakeSuccess();
                ChangeStatus(WxBoxClientStatus::ConnectWxBoxServerSuccess);
            }
            return true;
        default: {
            wxbot::WxBotMessage msg(wxbot::MsgRole::WxBox, wxbot::WxBotMessageType::WxBoxRequestOrResponse);
            msg.u.wxBoxControlPacket = packet;
            PushMessage(std::move(msg));
            break;
        }
    }

    return true;
}

void wxbot::WxBoxClient::WaitForEndPointFinish()
{
    if (!endpoint) {
        return;
    }

    WxBotMessage msg(MsgRole::UnknownRole, WxBotMessageType::UnknownMsgType);

    for (;;) {
        if (!endpoint->IsFinish()) {
            endpoint->Finish();
        }

        {
            std::unique_lock<std::shared_mutex> lock(rwmutex);
            cv.wait(lock, [&] {
                return queue.size();
            });

            msg = queue.front();
            queue.pop_front();
        }

        msg.Clear();

        if (msg.type == WxBotMessageType::WxBotConnectionLost) {
            endpoint = nullptr;
            ChangeStatus(WxBoxClientStatus::ConnectionLost);
            break;
        }
    }
}

wxbot::MessageLoopResult wxbot::WxBoxClient::MessageLoop()
{
    bool                     isTimeToExit = false;
    wxbot::MessageLoopResult result       = wxbot::MessageLoopResult::Done;
    WxBotMessage             msg(MsgRole::UnknownRole, WxBotMessageType::UnknownMsgType);

    for (;;) {
        //
        // critical section, pop a message from the queue front
        //

        {
            std::unique_lock<std::shared_mutex> lock(rwmutex);
            cv.wait(lock, [&] {
                return queue.size() || IsTimeToExit();
            });

            isTimeToExit = IsTimeToExit();

            if (!isTimeToExit && queue.size()) {
                msg = std::move(queue.front());
                queue.pop_front();
            }
        }

        if (isTimeToExit) {
            WaitForEndPointFinish();
            result = wxbot::MessageLoopResult::Done;
            break;
        }

        if (msg.type == WxBotMessageType::WxBotRetryConnect) {
            std::this_thread::sleep_for(msRetryInterval);
            result = IsTimeToExit() ? wxbot::MessageLoopResult::Done : wxbot::MessageLoopResult::ReConnect;
            msg.Clear();
            break;
        }
        else if (msg.type == WxBotMessageType::WxBotConnectionLost) {
            if (endpoint) {
                endpoint = nullptr;
            }
            result = wxbot::MessageLoopResult::ReConnect;
            ChangeStatus(WxBoxClientStatus::ConnectionLost);
            msg.Clear();
            break;
        }

        switch (msg.type) {
            case WxBotMessageType::WxBotRequest:
                SendPacket(msg.u.wxBotControlPacket);
                break;
            case WxBotMessageType::WxBotResponse:
                SendPacket(msg.u.wxBotControlPacket);
                break;
            case WxBotMessageType::WxBoxRequestOrResponse:
                PutMessageToWxBot(msg);
                break;
        }

        msg.Clear();
    }

    ClearMessageQueue();
    return result;
}

void wxbot::WxBoxClient::Routine()
{
    ChangeStatus(WxBoxClientStatus::Started);

    // set thread name
    wb_process::SetThreadName(wb_process::GetCurrentThreadHandle(), "WxBoxClient");

_RECONNECT:
    if (!TryConnectWxBoxServer()) {
        ChangeStatus(WxBoxClientStatus::ConnectWxBoxServerFailed);
        RetryConnect();
    }

    if (MessageLoop() == wxbot::MessageLoopResult::ReConnect) {
        ChangeStatus(WxBoxClientStatus::DoReConnect);
        ResetChannel();
        goto _RECONNECT;
    }

    ChangeStatus(WxBoxClientStatus::Stopped);
    running.store(false);
    doneSignal.set_value();
}