#ifndef ROCKET_NET_RPC_RPC_CHANNEL_H
#define ROCKET_NET_RPC_RPC_CHANNEL_H

#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/timer_event.h"
#include "rocket/common/error_coder.h"
#include <google/protobuf/service.h>
#include <memory>

namespace rocket {

#define NEWMESSAGE(type, var_name) std::shared_ptr<type> var_name = std::make_shared<type>();

#define NEWRPCONROLLER(var_name)                                                                                       \
    std::shared_ptr<rocket::RpcController> var_name = std::make_shared<rocket::RpcController>();

#define NEWRPGCHANNEEL(addr, var_name)                                                                                 \
    std::shared_ptr<rocket::RpcChannel> var_name =                                                                     \
        std::make_shared<rocket::RpcChannel>(std::make_shared<rocket::IPNetAddr>(addr));

std::string getMethodAddr(std::string method_path);

#define CALLRCP(stub_name, method_name, controller, request, response, closure)                                        \
    {                                                                                                                  \
        std::string method_path = "/" + std::string(#stub_name) + "/" + std::string(#method_name);                     \
        std::string addr = rocket::getMethodAddr(method_path);                                                         \
        if (addr == "") {                                                                                              \
            controller->SetError(ERROR_FAILED_GET_METHOD_ADDR, "get method address error");                            \
            return;                                                                                                    \
        }                                                                                                              \
        NEWRPGCHANNEEL(addr, channel);                                                                                 \
        channel->Init(controller, request, response, closure);                                                         \
        stub_name(channel.get()).method_name(controller.get(), request.get(), response.get(), closure.get());          \
    }

class RpcChannel : public google::protobuf::RpcChannel, public std::enable_shared_from_this<RpcChannel> {

public:
    typedef std::shared_ptr<RpcChannel> s_ptr;
    typedef std::shared_ptr<google::protobuf::RpcController> controller_s_ptr;
    typedef std::shared_ptr<google::protobuf::Message> message_s_ptr;
    typedef std::shared_ptr<google::protobuf::Closure> closure_s_ptr;

public:
    RpcChannel(NetAddr::s_ptr peer_addr);

    ~RpcChannel();

    void Init(controller_s_ptr controller, message_s_ptr req, message_s_ptr res, closure_s_ptr done);

    virtual void CallMethod(const google::protobuf::MethodDescriptor *method,
                            google::protobuf ::RpcController *controller, const google::protobuf::Message *request,
                            google::protobuf::Message *response, google::protobuf::Closure *done);

    google::protobuf::RpcController *getController();
    
    google::protobuf::Message *getRequest();

    google::protobuf::Message *getResponse();

    google::protobuf::Closure *getClosure();

    TcpClient* getTcpClient();

    TimerEvent::s_ptr getTimerEvent();

private:
    NetAddr::s_ptr m_peer_addr{nullptr};
    NetAddr::s_ptr m_local_addr{nullptr};

    controller_s_ptr m_controller{nullptr};
    message_s_ptr m_request{nullptr};
    message_s_ptr m_response{nullptr};
    closure_s_ptr m_closure{nullptr};

    bool m_is_init{false};

    TcpClient::s_ptr m_client {nullptr};

    TimerEvent::s_ptr m_timer_event {nullptr};
};

} // namespace rocket
#endif