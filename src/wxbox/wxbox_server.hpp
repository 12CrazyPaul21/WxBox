#ifndef __WXBOX_SERVER_H
#define __WXBOX_SERVER_H

//
// C/C++ headers
//

#include <string>
#include <memory>

//
// Qt Headers
//

#include <QObject>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QMainWindow>

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

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using wxbox::WxBox;
using wxbox::WxBoxRequest;
using wxbox::WxBoxResponse;

namespace wxbox {

    typedef enum _WxBoxServerStatus
    {
        Uninit = 0,
        Started,
        StartServiceFailed,
        Interrupted,
        Stopped
    } WxBoxServerStatus;

    Q_DECLARE_METATYPE(WxBoxServerStatus);

    class WxBoxServer Q_DECL_FINAL : public QObject, public WxBox::Service
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

        Status SayHello(ServerContext* context, const WxBoxRequest* request, WxBoxResponse* response) override
        {
            Q_UNUSED(context);

            std::string prefix("Hello ");
            response->set_message(prefix + request->name());
            return Status::OK;
        }

        //
        // Service Metadatas
        //

        static std::string WxBoxServerURI()
        {
            std::string uri = wxbox::WxBoxRequest::descriptor()->file()->options().GetExtension(wxbox::WxBoxServerURI);
            return uri.empty() ? "localhost:52333" : uri;
        }

      private:
        //
        // Private Status Methods
        //

        bool buildAndStartServer()
        {
            GOOGLE_PROTOBUF_VERIFY_VERSION;
            grpc::EnableDefaultHealthCheckService(true);
            grpc::reflection::InitProtoReflectionServerBuilderPlugin();
            serverImpl = ServerBuilder().AddListeningPort(WxBoxServerURI(), grpc::InsecureServerCredentials()).RegisterService(this).BuildAndStart();
            return serverImpl != nullptr;
        }

        void changeStatus(const WxBoxServerStatus& newStatus)
        {
            auto oldStatus = status;
            status         = newStatus;
            emit WxBoxServerStatusChange(oldStatus, newStatus);
        }

      signals:
        void WxBoxServerStatusChange(const WxBoxServerStatus oldStatus, const WxBoxServerStatus newStatus);

      public slots:

        void routine()
        {
            if (serverImpl) {
                goto _Finish;
            }

            if (!buildAndStartServer()) {
                changeStatus(WxBoxServerStatus::StartServiceFailed);
                goto _Finish;
            }

            if (QThread::currentThread()->isInterruptionRequested()) {
                shutdown();
                changeStatus(WxBoxServerStatus::Interrupted);
                goto _Finish;
            }

            changeStatus(WxBoxServerStatus::Started);
            serverImpl->Wait();
            changeStatus(WxBoxServerStatus::Stopped);

        _Finish:
            QThread::currentThread()->quit();
        }

        void shutdown()
        {
            if (serverImpl) {
                serverImpl->Shutdown();
            }
        }

        //
        // Event Slot
        //

        void WxBoxServerEvent()
        {
        }

      private:
        WxBoxServerStatus       status;
        std::unique_ptr<Server> serverImpl;
    };

    class WxBoxServerWorker : public QThread
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
            emit shutdown();
            requestInterruption();
            quit();
        }

      signals:
        void shutdown();
    };
}

#endif  // #ifndef __WXBOX_SERVER_H