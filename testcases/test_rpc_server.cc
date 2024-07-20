#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <assert.h>
#include <memory>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <google/protobuf/service.h>
#include "rocket/common/config.h"
#include "rocket/common/log.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"
#include "order.pb.h"
#include"rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_server.h"
#include "rocket/net/rpc/rpc_dispatcher.h"

class OrderImpl : public Order {
public:
    void makeOrder(google::protobuf::RpcController *controller, const ::makeOrderRequest *request,
                   ::makeOrderResponse *response, ::google::protobuf::Closure *done) {
        
        APPDEBUGLOG("start sleep 5s");
        sleep(5);
        APPDEBUGLOG("end sleep 5s");

        if (request->price() < 10) {
            response->set_ret_code(-1);
            response->set_res_info("short balance");
            return;
        }
        response->set_order_id("20230514");
        APPDEBUGLOG("call makeOrder sucess");
    }
};

void test_tcp_server() {
   rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);
   
   DEBUGLOG("create addr %s",addr->toString().c_str());

   rocket::TcpServer tcp_server(addr);

   tcp_server.start();
   
}

int main() {

    rocket::Config::SetGlobalConfig("../conf/rocket.xml");

    rocket::Logger::InitGlobalLogger();

    std::shared_ptr<OrderImpl>service=std::make_shared<OrderImpl>();
    rocket::RpcDispatcher::GetRpcDispatcher()->registerService(service);

    // test_connect();

    test_tcp_server();

    return 0;
}