#ifndef __WXBOX_SERVER_H
#define __WXBOX_SERVER_H

//
// C/C++ headers
//

#include <string>
#include <deque>
#include <memory>
#include <shared_mutex>
#include <condition_variable>

//
// Qt Headers
//

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>

//
// gRPC headers
//

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

//
// protobuf headers
//

#include <wxbox.grpc.pb.h>

//
// utils/internal headers
//

#undef signals
#include <utils/common.h>
#define signals Q_SIGNALS
#include <internal/threadpool.hpp>

using grpc::Server;
using grpc::ServerBidiReactor;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

using wxbox::WxBotControlPacket;
using wxbox::WxBox;
using wxbox::WxBoxControlPacket;
using wxbox::WxBoxHelloRequest;
using wxbox::WxBoxHelloResponse;

namespace wxbox {

#define WXBOX_SERVER_DEFAULT_URI "localhost:52333"

    typedef enum class _WxBoxServerStatus
    {
        Uninit = 0,
        Started,
        StartServiceFailed,
        Interrupted,
        Stopped
    } WxBoxServerStatus;

    typedef enum class _MsgRole
    {
        UnknownRole = 0,
        WxBox,
        WxBot
    } MsgRole;

    typedef enum class _WxBoxMessageType
    {
        UnknownMsgType = 0,
        WxBotRequestOrResponse,
        WxBoxRequest,
        WxBoxResponse,
        WxBoxClientConnected,
        WxBoxClientDone
    } WxBoxMessageType;

    typedef struct _WxBoxMessage
    {
        MsgRole          role;
        WxBoxMessageType type;
        wb_process::PID  pid;
        union _u
        {
            WxBotControlPacket wxBotControlPacket;
            WxBoxControlPacket wxBoxControlPacket;

            _u() /* = delete */
            {
            }

            _u(const WxBoxMessageType type)
            {
                constructor(type);
            }

            ~_u()
            {
                // do nothing
            }

            void constructor(const WxBoxMessageType type)
            {
                U_OBJ_CONSTRUCTOR(WxBoxMessageType::WxBotRequestOrResponse, wxBotControlPacket, WxBotControlPacket);
                U_OBJ_CONSTRUCTOR(WxBoxMessageType::WxBoxRequest, wxBoxControlPacket, WxBoxControlPacket);
                U_OBJ_CONSTRUCTOR(WxBoxMessageType::WxBoxResponse, wxBoxControlPacket, WxBoxControlPacket);
            }

            void destructor(const WxBoxMessageType type)
            {
                U_OBJ_DESTRUCTOR(WxBoxMessageType::WxBotRequestOrResponse, wxBotControlPacket, WxBotControlPacket);
                U_OBJ_DESTRUCTOR(WxBoxMessageType::WxBoxRequest, wxBoxControlPacket, WxBoxControlPacket);
                U_OBJ_DESTRUCTOR(WxBoxMessageType::WxBoxResponse, wxBoxControlPacket, WxBoxControlPacket);
            }

            void copy(const WxBoxMessageType type, const _u& other)
            {
                U_OBJ_COPY(WxBoxMessageType::WxBotRequestOrResponse, wxBotControlPacket, WxBotControlPacket, other);
                U_OBJ_COPY(WxBoxMessageType::WxBoxRequest, wxBoxControlPacket, WxBoxControlPacket, other);
                U_OBJ_COPY(WxBoxMessageType::WxBoxResponse, wxBoxControlPacket, WxBoxControlPacket, other);
            }

            void move(const WxBoxMessageType type, _u&& other)
            {
                U_OBJ_MOVE(WxBoxMessageType::WxBotRequestOrResponse, wxBotControlPacket, WxBotControlPacket, other);
                U_OBJ_MOVE(WxBoxMessageType::WxBoxRequest, wxBoxControlPacket, WxBoxControlPacket, other);
                U_OBJ_MOVE(WxBoxMessageType::WxBoxResponse, wxBoxControlPacket, WxBoxControlPacket, other);
            }
        } u;

        _WxBoxMessage()
          : role(MsgRole::UnknownRole)
          , type(WxBoxMessageType::UnknownMsgType)
          , pid(0)
          , u(WxBoxMessageType::UnknownMsgType)
        {
        }

        _WxBoxMessage(const MsgRole role, const WxBoxMessageType type)
          : role(role)
          , type(type)
          , pid(0)
          , u(type)
        {
        }

        ~_WxBoxMessage()
        {
            u.destructor(type);
        }

        SETUP_COPY_METHOD(_WxBoxMessage, other)
        {
            role = other.role;
            type = other.type;
            pid  = other.pid;
            u.copy(type, other.u);
        }

        SETUP_MOVE_METHOD(_WxBoxMessage, other)
        {
            role = other.role;
            type = other.type;
            pid  = other.pid;
            u.move(type, std::move(other.u));
        }

        void Clear()
        {
            switch (type) {
                case WxBoxMessageType::WxBotRequestOrResponse:
                    u.wxBotControlPacket.Clear();
                    break;
                case WxBoxMessageType::WxBoxRequest:
                case WxBoxMessageType::WxBoxResponse:
                    u.wxBoxControlPacket.Clear();
                    break;
            }
        }

    } WxBoxMessage, *PWxBoxMessage;

    Q_DECLARE_METATYPE(WxBoxServerStatus);
    Q_DECLARE_METATYPE(MsgRole);
    Q_DECLARE_METATYPE(WxBoxMessageType);
    Q_DECLARE_METATYPE(WxBoxMessage);

    class WxBotEndPoint : public grpc::ServerBidiReactor<WxBotControlPacket, WxBoxControlPacket>
    {
        friend class WxBoxServer;

      public:
        explicit WxBotEndPoint(WxBoxServer* server)
          : server(server)
          , hold(false)
          , finished(false)
        {
            StartRead(&fromClientPacket);
        }

        void OnDone() override;
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
                Finish(Status::OK);
                return;
            }

            NextPacket();
        }

        void OnCancel() override
        {
            // Finish(grpc::Status::OK);
        }

        void Finish(grpc::Status status) ABSL_LOCKS_EXCLUDED(stream_mu_m)
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

            grpc::ServerBidiReactor<WxBotControlPacket, WxBoxControlPacket>::Finish(status);
        }

        bool SendPacket(const wxbox::WxBoxControlPacket& packet)
        {
            if (finished) {
                return false;
            }

            std::unique_lock<std::mutex> lock(mutex);
            queue.push_back(packet);
            if (queue.size() == 1) {
                StartWrite(&queue[0]);
            }

            return true;
        }

        void MarkRegister()
        {
            hold = true;
        }

        bool IsRegistered() const
        {
            return hold;
        }

      private:
        void NextPacket()
        {
            if (finished) {
                return;
            }

            std::unique_lock<std::mutex> lock(mutex);
            if (queue.size()) {
                StartWrite(&queue[0]);
            }
        }

      private:
        std::atomic<bool>                     finished;
        std::atomic<bool>                     hold;
        wxbox::WxBotControlPacket             fromClientPacket;
        std::deque<wxbox::WxBoxControlPacket> queue;
        std::mutex                            mutex;
        class wxbox::WxBoxServer*             server;
    };

    class WxBoxServer Q_DECL_FINAL : public QObject, public WxBox::CallbackService
    {
        Q_OBJECT

      public:
        explicit WxBoxServer(QObject* parent = nullptr)
          : QObject(parent)
          , status(WxBoxServerStatus::Uninit)
          , serverImpl(nullptr)
        {
        }

        static WxBoxServer* NewWxBoxServer(QObject* parent = nullptr)
        {
            return new WxBoxServer(parent);
        }

        //
        // RPC Methods
        //

        virtual grpc::ServerUnaryReactor* Ping(grpc::CallbackServerContext* context, const WxBoxHelloRequest* request, WxBoxHelloResponse* response) override
        {
            Q_UNUSED(request);

            auto* reactor = context->DefaultReactor();
            response->set_pong(true);
            reactor->Finish(Status::OK);
            return reactor;
        }

        virtual grpc::ServerBidiReactor<WxBotControlPacket, WxBoxControlPacket>* Communication(grpc::CallbackServerContext* context) override
        {
            Q_UNUSED(context);

            return CreateWxBotEndPoint();
        }

        //
        // Service Metadatas
        //

        static std::string WxBoxServerURI()
        {
            std::string uri = wxbox::WxBoxHelloRequest::descriptor()->file()->options().GetExtension(wxbox::WxBoxServerURI);
            return uri.empty() ? WXBOX_SERVER_DEFAULT_URI : uri;
        }

        //
        // Client Methods
        //

        WxBotEndPoint* CreateWxBotEndPoint()
        {
            WxBotEndPoint* endpoint = new WxBotEndPoint(this);
            {
                std::unique_lock<std::shared_mutex> lock(mutex);
                waitingList.push_back(endpoint);
            }
            return endpoint;
        }

        WxBotEndPoint* GetWxBotEndPointByPID(wb_process::PID pid)
        {
            WxBotEndPoint* endpoint = nullptr;
            {
                std::shared_lock<std::shared_mutex> lock(mutex);
                if (clients.find(pid) != clients.end()) {
                    endpoint = clients[pid];
                }
            }
            return endpoint;
        }

        void     ResponseHandshake(wb_process::PID pid, bool success);
        bool     RegisterWxBotEndPoint(wb_process::PID pid, WxBotEndPoint* endpoint);
        void     CloseAllClients();
        bool     IsConnectionEmpty() const;
        uint64_t GetClientCounts() const;

        bool SendPacket(wb_process::PID pid, const wxbox::WxBoxControlPacket& packet);

        //
        // Message Methods
        //

        void HandleWxBoxClientFinish(WxBotEndPoint* endpoint);
        bool HandleWxBoxClientRequest(WxBotEndPoint* endpoint, wxbox::WxBotControlPacket request);

        void PushMessage(WxBoxMessage message)
        {
            {
                std::unique_lock<std::shared_mutex> lock(rwmutex);
                queue.emplace_back(std::move(message));
            }
            cv.notify_one();
        }

        Q_SLOT void PushMessageAsync(WxBoxMessage message)
        {
            wxbox::internal::TaskInThreadPool::StartTask([this, message]() {
                PushMessage(std::move(message));
            });
        }

        void PutMessageToWxBox(const WxBoxMessage& message)
        {
            emit WxBoxServerEvent(message);
        }

      private:
        //
        // Private Status Methods
        //

        bool BuildAndStartServer()
        {
            GOOGLE_PROTOBUF_VERIFY_VERSION;
            grpc::EnableDefaultHealthCheckService(true);
            grpc::reflection::InitProtoReflectionServerBuilderPlugin();
            serverImpl = ServerBuilder().AddListeningPort(WxBoxServerURI(), grpc::InsecureServerCredentials()).RegisterService(this).BuildAndStart();
            return serverImpl != nullptr;
        }

        void ChangeStatus(const WxBoxServerStatus& newStatus)
        {
            auto oldStatus = status;
            status         = newStatus;
            emit WxBoxServerStatusChange(oldStatus, newStatus);
        }

        //
        // Message Loop
        //

        void Wait();
        void MessageLoop();

      signals:
        void WxBoxServerStatusChange(const WxBoxServerStatus oldStatus, const WxBoxServerStatus newStatus);
        void WxBoxServerEvent(WxBoxMessage message);

      public slots:

        void routine()
        {
            if (serverImpl) {
                goto _Finish;
            }

            if (!BuildAndStartServer()) {
                ChangeStatus(WxBoxServerStatus::StartServiceFailed);
                goto _Finish;
            }

            if (QThread::currentThread()->isInterruptionRequested()) {
                serverImpl->Shutdown();
                ChangeStatus(WxBoxServerStatus::Interrupted);
                goto _Finish;
            }

            // set thread name
            wb_process::SetThreadName(wb_process::GetCurrentThreadHandle(), "WxBoxServer");

            ChangeStatus(WxBoxServerStatus::Started);
            MessageLoop();
            ChangeStatus(WxBoxServerStatus::Stopped);

        _Finish:
            QThread::currentThread()->quit();
        }

        void shutdown()
        {
            if (serverImpl) {
                wxbox::internal::TaskInThreadPool::StartTask([=]() {
                    serverImpl->Shutdown();
                });
            }
            cv.notify_one();
        }

      private:
        WxBoxServerStatus                                   status;
        std::unique_ptr<Server>                             serverImpl;
        mutable std::shared_mutex                           mutex;
        mutable std::shared_mutex                           rwmutex;
        mutable std::condition_variable_any                 cv;
        std::deque<WxBoxMessage>                            queue;
        std::vector<WxBotEndPoint*>                         waitingList;
        std::unordered_map<wb_process::PID, WxBotEndPoint*> clients;
    };

    class WxBoxServerWorker Q_DECL_FINAL : public QThread
    {
        Q_OBJECT

      public:
        explicit WxBoxServerWorker(QObject* parent = nullptr)
          : QThread(parent)
        {
        }

        void startServer(WxBoxServer* server, QThread::Priority priority = InheritPriority)
        {
            if (!server) {
                return;
            }

            server->moveToThread(this);
            start(priority);
            QMetaObject::invokeMethod(server, "routine", Qt::QueuedConnection);
        }

        void stopServer()
        {
            requestInterruption();
            emit shutdown();
            quit();
        }

      signals:
        void shutdown();
    };
}

#endif  // #ifndef __WXBOX_SERVER_H