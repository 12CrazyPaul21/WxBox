#ifndef __WXBOX_CLIENT_H
#define __WXBOX_CLIENT_H

//
// C/C++ headers
//

#include <atomic>
#include <chrono>
#include <memory>
#include <vector>
#include <deque>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <future>
#include <functional>
#include <condition_variable>

//
// Third party headers
//

#include <grpcpp/grpcpp.h>

//
// WxBot headers
//

#include <utils/common.h>
#include <wxbox.grpc.pb.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using wxbox::WxBotControlPacket;
using wxbox::WxBox;
using wxbox::WxBoxControlPacket;
using wxbox::WxBoxHelloRequest;
using wxbox::WxBoxHelloResponse;

namespace wxbot {

#define WXBOX_SERVER_DEFAULT_URI "localhost:52333"
#define WXBOX_CLIENT_CONNECT_RETRY_INTERVAL_MS 500

    typedef enum class _MessageLoopResult
    {
        Done = 0,
        ReConnect
    } MessageLoopResult;

    typedef enum class _MsgRole
    {
        UnknownRole = 0,
        WxBox,
        WxBot
    } MsgRole;

    typedef enum class _WxBotMessageType
    {
        UnknownMsgType = 0,
        WxBoxRequestOrResponse,
        WxBotRequest,
        WxBotResponse,
        WxBotRetryConnect,
        WxBotConnectionLost,
        WxBoxClientStatusChange
    } WxBotMessageType;

    typedef enum class _WxBoxClientStatus
    {
        Uninit = 0,
        Started,
        ConnectWxBoxServerFailed,
        ConnectWxBoxServerSuccess,
        ConnectionLost,
        DoReConnect,
        Stopped
    } WxBoxClientStatus;

    typedef struct _WxBoxClientStatusChangeInfo
    {
        WxBoxClientStatus oldStatus;
        WxBoxClientStatus newStatus;

        _WxBoxClientStatusChangeInfo()
        {
        }

        _WxBoxClientStatusChangeInfo(WxBoxClientStatus oldStatus, WxBoxClientStatus newStatus)
          : oldStatus(oldStatus)
          , newStatus(newStatus)
        {
        }
    } WxBoxClientStatusChangeInfo;

    typedef struct _WxBotMessage
    {
        MsgRole          role;
        WxBotMessageType type;
        union _u
        {
            WxBotControlPacket          wxBotControlPacket;
            WxBoxControlPacket          wxBoxControlPacket;
            WxBoxClientStatusChangeInfo wxBoxClientStatus;

            _u() /* = delete */
            {
            }

            _u(const WxBotMessageType type)
            {
                constructor(type);
            }

            ~_u()
            {
                // do nothing
            }

            void constructor(const WxBotMessageType type)
            {
                U_OBJ_CONSTRUCTOR(WxBotMessageType::WxBoxRequestOrResponse, wxBoxControlPacket, WxBoxControlPacket);
                U_OBJ_CONSTRUCTOR(WxBotMessageType::WxBotRequest, wxBotControlPacket, WxBotControlPacket);
                U_OBJ_CONSTRUCTOR(WxBotMessageType::WxBotResponse, wxBotControlPacket, WxBotControlPacket);
                U_SCALAR_CONSTRUCTOR(WxBotMessageType::WxBoxClientStatusChange, wxBoxClientStatus, WxBoxClientStatusChangeInfo(WxBoxClientStatus::Uninit, WxBoxClientStatus::Uninit));
            }

            void destructor(const WxBotMessageType type)
            {
                U_OBJ_DESTRUCTOR(WxBotMessageType::WxBoxRequestOrResponse, wxBoxControlPacket, WxBoxControlPacket);
                U_OBJ_DESTRUCTOR(WxBotMessageType::WxBotRequest, wxBotControlPacket, WxBotControlPacket);
                U_OBJ_DESTRUCTOR(WxBotMessageType::WxBotResponse, wxBotControlPacket, WxBotControlPacket);
            }

            void copy(const WxBotMessageType type, const _u& other)
            {
                U_OBJ_COPY(WxBotMessageType::WxBoxRequestOrResponse, wxBoxControlPacket, WxBoxControlPacket, other);
                U_OBJ_COPY(WxBotMessageType::WxBotRequest, wxBotControlPacket, WxBotControlPacket, other);
                U_OBJ_COPY(WxBotMessageType::WxBotResponse, wxBotControlPacket, WxBotControlPacket, other);
                U_SCALAR_COPY(WxBotMessageType::WxBoxClientStatusChange, wxBoxClientStatus, other);
            }

            void move(const WxBotMessageType type, _u&& other)
            {
                U_OBJ_MOVE(WxBotMessageType::WxBoxRequestOrResponse, wxBoxControlPacket, WxBoxControlPacket, other);
                U_OBJ_MOVE(WxBotMessageType::WxBotRequest, wxBotControlPacket, WxBotControlPacket, other);
                U_OBJ_MOVE(WxBotMessageType::WxBotResponse, wxBotControlPacket, WxBotControlPacket, other);
                U_SCALAR_COPY(WxBotMessageType::WxBoxClientStatusChange, wxBoxClientStatus, other);
            }
        } u;

        _WxBotMessage()
          : role(MsgRole::UnknownRole)
          , type(WxBotMessageType::UnknownMsgType)
          , u(WxBotMessageType::UnknownMsgType)
        {
        }

        _WxBotMessage(const MsgRole role, const WxBotMessageType type)
          : role(role)
          , type(type)
          , u(type)
        {
        }

        ~_WxBotMessage()
        {
            u.destructor(type);
        }

        SETUP_COPY_METHOD(_WxBotMessage, other)
        {
            role = other.role;
            type = other.type;
            u.copy(type, other.u);
        }

        SETUP_MOVE_METHOD(_WxBotMessage, other)
        {
            role = other.role;
            type = other.type;
            u.move(type, std::move(other.u));
        }

        void Clear()
        {
            switch (type) {
                case WxBotMessageType::WxBoxRequestOrResponse:
                    u.wxBoxControlPacket.Clear();
                    break;
                case WxBotMessageType::WxBotRequest:
                case WxBotMessageType::WxBotResponse:
                    u.wxBotControlPacket.Clear();
                    break;
            }
        }

    } WxBotMessage, *PWxBotMessage;

    using WxBoxClientCallback = std::function<void(WxBotMessage)>;

    class WxBoxEndPoint final : public grpc::ClientBidiReactor<WxBotControlPacket, WxBoxControlPacket>
    {
      public:
        friend class WxBoxClient;

        explicit WxBoxEndPoint(WxBox::Stub* stub, WxBoxClient* client)
          : client(client)
          , finished(false)
        {
            if (!stub) {
                return;
            }

            stub->async()->Communication(&context, this);
            context.set_wait_for_ready(false);
            AddHold();
            StartRead(&fromServerPacket);
            StartCall();
        }

        void OnDone(const Status& status) override;
        void OnReadDone(bool ok) override;

        void OnWriteDone(bool ok) override
        {
            {
                std::unique_lock<std::mutex> lock(mutex);
                if (queue.size()) {
                    queue.pop_front();
                }
            }

            if (!ok) {
                Finish();
                return;
            }

            NextPacket();
        }

        bool IsFinish() const
        {
            return finished.load();
        }

        void Finish()
        {
            bool alreadyFinished = false;
            finished.compare_exchange_strong(alreadyFinished, true);

            if (alreadyFinished) {
                return;
            }

            {
                std::unique_lock<std::mutex> lock(mutex);
                queue.clear();
            }

            RemoveHold();
            context.TryCancel();
        }

        bool SendPacket(const wxbox::WxBotControlPacket& packet)
        {
            if (finished.load()) {
                return false;
            }

            if (!communicable && packet.type() != wxbox::ControlPacketType::HANDSHAKE_REQUEST) {
                return false;
            }

            std::unique_lock<std::mutex> lock(mutex);
            queue.push_back(packet);
            if (queue.size() == 1) {
                queue[0].set_pid(wb_process::GetCurrentProcessId());
                StartWrite(&queue[0]);
            }

            return true;
        }

        bool Handshake()
        {
            wxbox::WxBotControlPacket request;
            request.set_type(wxbox::ControlPacketType::HANDSHAKE_REQUEST);
            return SendPacket(std::move(request));
        }

        void HandshakeSuccess()
        {
            communicable = true;
        }

      private:
        void NextPacket()
        {
            if (finished.load()) {
                return;
            }

            std::unique_lock<std::mutex> lock(mutex);
            if (queue.size()) {
                StartWrite(&queue[0]);
            }
        }

      private:
        std::atomic<bool>              finished;
        std::atomic<bool>              communicable;
        ClientContext                  context;
        WxBoxControlPacket             fromServerPacket;
        std::deque<WxBotControlPacket> queue;
        std::mutex                     mutex;
        class wxbot::WxBoxClient*      client;
    };

    class WxBoxClient final
    {
        friend class WxBoxEndPoint;

      private:
        WxBoxClient()                   = delete;
        WxBoxClient(const WxBoxClient&) = delete;
        WxBoxClient(WxBoxClient&&)      = delete;
        WxBoxClient* operator=(const WxBoxClient&) = delete;
        WxBoxClient* operator=(WxBoxClient&&) = delete;

      public:
        explicit WxBoxClient(const std::string& uri)
          : clientStatus(WxBoxClientStatus::Uninit)
          , serverURI(uri)
          , channel(nullptr)
          , stub(nullptr)
          , endpoint(nullptr)
          , running(false)
          , msRetryInterval(WXBOX_CLIENT_CONNECT_RETRY_INTERVAL_MS)
        {
            ResetChannel();
        }

        //
        // RPC Stub Methods
        //

        bool Ping()
        {
            ClientContext      context;
            WxBoxHelloRequest  request;
            WxBoxHelloResponse response;

            request.set_pid(wb_process::GetCurrentProcessId());
            Status status = stub->Ping(&context, request, &response);
            if (status.ok()) {
                return response.pong();
            }

            return false;
        }

        //
        // Client Methods
        //

        void ResetChannel();

        bool Start();
        void Stop();
        void Wait();

        void PushMessage(WxBotMessage message);
        void PushMessageAsync(WxBotMessage message);
        void ClearMessageQueue();
        void PutMessageToWxBot(const WxBotMessage& message);

        bool SendPacket(const WxBotControlPacket& packet);

        //
        // Property Methods
        //

        void ChangeServerURI(const std::string& uri)
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            serverURI = uri;
        }

        std::string GetServerURI() const
        {
            std::shared_lock<std::shared_mutex> lock(mutex);
            return serverURI;
        }

        void RegisterWxBotCallback(WxBoxClientCallback callback)
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            this->wxbotCallback = callback;
        }

        WxBoxClientCallback GetWxBotCallback() const
        {
            std::shared_lock<std::shared_mutex> lock(mutex);
            return this->wxbotCallback;
        }

        void SetRetryInterval(std::chrono::milliseconds::rep ms)
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            msRetryInterval = std::chrono::milliseconds(ms);
        }

        std::chrono::milliseconds::rep GetRetryInterval() const
        {
            std::shared_lock<std::shared_mutex> lock(mutex);
            return msRetryInterval.count();
        }

        //
        // Service Metadatas
        //

        static std::string WxBoxServerURI()
        {
            std::string uri = wxbox::WxBoxHelloRequest::descriptor()->file()->options().GetExtension(wxbox::WxBoxServerURI);
            return uri.empty() ? WXBOX_SERVER_DEFAULT_URI : uri;
        }

      private:
        //
        // Message Loop
        //

        void              ChangeStatus(WxBoxClientStatus status);
        void              RetryConnect();
        bool              TryConnectWxBoxServer();
        bool              IsTimeToExit();
        void              HandleWxBoxEndPointFinish();
        bool              HandleWxBoxServerRequest(WxBoxControlPacket packet);
        void              WaitForEndPointFinish();
        MessageLoopResult MessageLoop();
        void              Routine();

      private:
        std::string                         serverURI;
        WxBoxClientStatus                   clientStatus;
        std::chrono::milliseconds           msRetryInterval;
        std::thread                         worker;
        std::atomic_bool                    running;
        std::promise<void>                  exitSignal;
        std::promise<void>                  doneSignal;
        std::future<void>                   exitFuture;
        std::future<void>                   doneFuture;
        std::shared_ptr<Channel>            channel;
        std::unique_ptr<WxBox::Stub>        stub;
        std::unique_ptr<WxBoxEndPoint>      endpoint;
        WxBoxClientCallback                 wxbotCallback;
        mutable std::shared_mutex           mutex;
        mutable std::shared_mutex           rwmutex;
        mutable std::condition_variable_any cv;
        std::deque<WxBotMessage>            queue;
    };
}

#endif  // #ifndef __WXBOX_CLIENT_H