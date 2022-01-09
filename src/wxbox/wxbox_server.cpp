#include <wxbox_server.hpp>

//
// WxBotEndPoint
//

void wxbox::WxBotEndPoint::OnDone()
{
    if (!server) {
        return;
    }

    server->HandleWxBoxClientFinish(this);
}

void wxbox::WxBotEndPoint::OnReadDone(bool ok)
{
    if (!ok) {
        Finish(Status::OK);
        return;
    }

    bool failed = !server || !server->HandleWxBoxClientRequest(this, fromClientPacket);
    fromClientPacket.Clear();

    if (failed) {
        Finish(Status::OK);
        return;
    }

    StartRead(&fromClientPacket);
}

//
// WxBoxServer
//

void wxbox::WxBoxServer::ResponseHandshake(wb_process::PID pid, bool success)
{
    WxBoxMessage responseMsg(MsgRole::WxBox, WxBoxMessageType::WxBoxResponse);
    responseMsg.pid = pid;
    responseMsg.u.wxBoxControlPacket.set_type(wxbox::ControlPacketType::HANDSHAKE_RESPONSE);
    auto response = responseMsg.u.wxBoxControlPacket.mutable_handshakeresponse();
    response->set_success(success);
    PushMessage(std::move(responseMsg));
}

bool wxbox::WxBoxServer::RegisterWxBotEndPoint(wb_process::PID pid, WxBotEndPoint* endpoint)
{
    if (!endpoint) {
        return false;
    }

    if (endpoint->IsRegistered()) {
        ResponseHandshake(pid, false);
        return true;
    }

#ifndef _DEBUG
    if (!wb_wx::CheckWeChatProcessValid(pid)) {
        return false;
    }
#endif

    //
    // register client
    //

    {
        std::unique_lock<std::shared_mutex> lock(mutex);

        if (clients.find(pid) != clients.end()) {
            return false;
        }

        waitingList.erase(std::remove_if(waitingList.begin(), waitingList.end(), [endpoint](auto cursor) {
            return endpoint == cursor;
        }));

        clients[pid] = endpoint;
        endpoint->MarkRegister();
    }

    //
    // response success
    //

    ResponseHandshake(pid, true);

    //
    // notify WxBox
    //

    WxBoxMessage notifyMsg(MsgRole::WxBox, WxBoxMessageType::WxBoxClientConnected);
    notifyMsg.pid = pid;
    PushMessage(std::move(notifyMsg));

    return true;
}

void wxbox::WxBoxServer::CloseAllClients()
{
    {
        std::shared_lock<std::shared_mutex> lock(mutex);

        for (auto endpoint : waitingList) {
            if (endpoint) {
                endpoint->Finish(Status::OK);
            }
        }

        for (auto client : clients) {
            auto endpoint = client.second;
            if (endpoint) {
                endpoint->Finish(Status::OK);
            }
        }
    }
}

bool wxbox::WxBoxServer::IsConnectionEmpty() const
{
    {
        std::shared_lock<std::shared_mutex> lock(mutex);
        return waitingList.empty() && clients.empty();
    }
}

uint64_t wxbox::WxBoxServer::GetClientCounts() const
{
    {
        std::shared_lock<std::shared_mutex> lock(mutex);
        return clients.size();
    }
}

bool wxbox::WxBoxServer::SendPacket(wb_process::PID pid, const wxbox::WxBoxControlPacket& packet)
{
    if (!pid) {
        return false;
    }

    auto endpoint = GetWxBotEndPointByPID(pid);
    if (!endpoint) {
        return false;
    }

    if (endpoint->finished) {
        return false;
    }

    return endpoint->SendPacket(packet);
}

void wxbox::WxBoxServer::HandleWxBoxClientFinish(WxBotEndPoint* endpoint)
{
#define NOTIFY_AND_RETURN() \
    {                       \
        cv.notify_one();    \
        return;             \
    }

    if (!endpoint) {
        return;
    }

    wb_process::PID pid = 0;

    {
        std::unique_lock<std::shared_mutex> lock(mutex);

        // invalid endpoint
        auto lit = find(waitingList.begin(), waitingList.end(), endpoint);
        if (lit != waitingList.end()) {
            waitingList.erase(lit);
            delete endpoint;
            NOTIFY_AND_RETURN();
        }

        //
        // valid endpoint
        //

        auto mit = std::find_if(clients.begin(), clients.end(), [&](const std::unordered_map<wb_process::PID, WxBotEndPoint*>::value_type& pair) {
            if (pair.second == endpoint) {
                pid = pair.first;
                return true;
            }
            return false;
        });

        if (mit == clients.end()) {
            NOTIFY_AND_RETURN();
        }

        clients.erase(mit);
        delete endpoint;
    }

    // notify WxBox
    wxbox::WxBoxMessage msg(wxbox::MsgRole::WxBox, wxbox::WxBoxMessageType::WxBoxClientDone);
    msg.pid = pid;
    PushMessage(std::move(msg));
}

bool wxbox::WxBoxServer::HandleWxBoxClientRequest(WxBotEndPoint* endpoint, wxbox::WxBotControlPacket request)
{
    if (!endpoint) {
        return false;
    }

    auto type = request.type();
    auto pid  = request.pid();

    switch (type) {
        case wxbox::ControlPacketType::HANDSHAKE_REQUEST:
            return RegisterWxBotEndPoint(pid, endpoint);
        default: {
            wxbox::WxBoxMessage msg(wxbox::MsgRole::WxBot, wxbox::WxBoxMessageType::WxBotRequestOrResponse);
            msg.pid                  = pid;
            msg.u.wxBotControlPacket = std::move(request);
            PushMessage(std::move(msg));
            break;
        }
    }

    return true;
}

void wxbox::WxBoxServer::Wait()
{
    WxBoxMessage msg(UnknownRole, UnknownMsgType);

    // wait for connection empty
    while (!IsConnectionEmpty()) {
        {
            std::unique_lock<std::shared_mutex> lock(rwmutex);
            cv.wait(lock, [&] {
                return queue.size() || IsConnectionEmpty();
            });
            if (!queue.size()) {
                break;
            }

            msg = std::move(queue.front());
            queue.pop_front();
        }

        switch (msg.type) {
            case WxBoxClientDone:
                PutMessageToWxBox(msg);
                break;
        }

        msg.Clear();
    }

    // wait for gRPC Server ends
    serverImpl->Wait();
}

void wxbox::WxBoxServer::MessageLoop()
{
    WxBoxMessage msg(UnknownRole, UnknownMsgType);

    for (;;) {
        //
        // critical section, pop a message from the queue front
        //

        {
            std::unique_lock<std::shared_mutex> lock(rwmutex);
            cv.wait(lock, [&] {
                return queue.size() || QThread::currentThread()->isInterruptionRequested();
            });
            if (QThread::currentThread()->isInterruptionRequested()) {
                break;
            }

            msg = std::move(queue.front());
            queue.pop_front();
        }

        //
        // handle message
        //

        switch (msg.type) {
            case WxBoxResponse:
                SendPacket(msg.pid, msg.u.wxBoxControlPacket);
                break;
            case WxBoxRequest:
                SendPacket(msg.pid, msg.u.wxBoxControlPacket);
                break;
            case WxBotRequestOrResponse:
                PutMessageToWxBox(msg);
                break;
            case WxBoxClientConnected:
                PutMessageToWxBox(msg);
                break;
            case WxBoxClientDone:
                PutMessageToWxBox(msg);
                break;
        }

        msg.Clear();
    }

    // close all clients
    CloseAllClients();

    // wait for server ends
    Wait();
}