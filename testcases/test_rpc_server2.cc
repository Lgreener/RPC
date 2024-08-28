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
#include "rocket/common/zookeeper_util.h"

class OrderImpl : public Order {
public:
    void makeOrder(google::protobuf::RpcController *controller, const ::makeOrderRequest *request,
                   ::makeOrderResponse *response, ::google::protobuf::Closure *done) {
        
        // APPDEBUGLOG("start sleep 5s");
        // sleep(5);
        // APPDEBUGLOG("end sleep 5s");

        if (request->price() < 10) {
            response->set_ret_code(-1);
            response->set_res_info("short balance");
            APPDEBUGLOG("call makeOrder sucess");
            return;
        }
        response->set_order_id("20230514");
        APPDEBUGLOG("call makeOrder sucess");
    }
};

int main(int argc, char* argv[]) {

    if (argc != 2) {
        printf("start test_rpc_server error,argc not2\n");
        printf("Start like this:\n");
        printf("./test_rpc_server ../conf/rocket.xml\n");
        return 0;
    }

    rocket::Config::SetGlobalConfig(argv[1]);

    rocket::Logger::InitGlobalLogger();

    std::shared_ptr<OrderImpl>service=std::make_shared<OrderImpl>();
    rocket::RpcDispatcher::GetRpcDispatcher()->registerService(service);
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.2", 12345);
    rocket::ZkClient zkCli;
    zkCli.Start();
    std::string service_path =  "/Order_Stub" ;
    zkCli.Create(service_path.c_str(), nullptr, 0);
    std::string method_path = service_path + "/makeOrder";
    zkCli.Create(method_path.c_str(), nullptr, 0);
    std::string addr_path = method_path + "/" + addr->toString();
    zkCli.Create(addr_path.c_str(), addr->toString().c_str(),strlen(addr->toString().c_str()), ZOO_EPHEMERAL);

    rocket::TcpServer tcp_server(addr);

    tcp_server.start();

    return 0;
}