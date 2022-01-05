#include <frida-gum.h>
#include <lua.hpp>
#include <spdlog/spdlog.h>
#include <wxbot.hpp>

#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include <wxbox.grpc.pb.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using wxbox::WxBox;
using wxbox::WxBoxRequest;
using wxbox::WxBoxResponse;

class WxBoxClient
{
  public:
    WxBoxClient(std::shared_ptr<Channel> channel)
      : stub(WxBox::NewStub(channel))
    {
    }

    std::string SayHello(const std::string& user)
    {
        WxBoxRequest request;
        request.set_name(user);

        WxBoxResponse response;
        ClientContext context;

        Status status = stub->SayHello(&context, request, &response);
        if (status.ok()) {
            return response.message();
        }
        else {
            return "failed";
        }
    }

    static std::string WxBoxServerURI()
    {
        std::string uri = wxbox::WxBoxRequest::descriptor()->file()->options().GetExtension(wxbox::WxBoxServerURI);
        return uri.empty() ? "localhost:52333" : uri;
    }

  private:
    std::unique_ptr<WxBox::Stub> stub;
};

namespace wxbot {

    Wxbot::Wxbot()
    {
        number = 6;
        spdlog::info("build wxbot");
        gum_init_embedded();
        lua_State* lua = luaL_newstate();
    }

    int Wxbot::get_number() const
    {
        std::string name = "wxbot";
        WxBoxClient client(grpc::CreateChannel(WxBoxClient::WxBoxServerURI(), grpc::InsecureChannelCredentials()));
        std::string reply = client.SayHello(name);
        spdlog::info("response {}", reply);
        return number;
    }

}  // namespace wxbot
