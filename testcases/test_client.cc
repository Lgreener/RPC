#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <assert.h>
#include <memory>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "rocket/common/config.h"
#include "rocket/common/log.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"

void test_connect() {
    //调用conenct连接 server
    // wirte一个字符串
    //等待read返回结果
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        ERRORLOG("invalid fd %d", fd);
        exit(0);
    }
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_aton("127.0.0.1", &server_addr.sin_addr);
    int rt = connect(fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr));
                     
    std::string msg = "hello rocket!";
    rt = write(fd, msg.c_str(), msg.length());
    INFOLOG("success write %d bytes,[%s]", rt, msg.c_str());

    char buf[100];
    rt = read(fd, buf, 100);

    INFOLOG("success read %d bytes,[%s]", rt, std::string(buf, rt).c_str());

    close(fd);
}

void test_tcp_client() {

    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);
    rocket::TcpClient client(addr);

    client.connect([addr, &client]() {
        DEBUGLOG("conenct to [%s] success", addr->toString().c_str())
        std::shared_ptr<rocket::TinyPBProtocol> message = std::make_shared<rocket::TinyPBProtocol>();
        message->m_msg_id  = "123456789";
        message->m_pb_data ="test pb_data";
        client.writeMessage(message, [](rocket::AbstractProtocol::s_ptr msg_ptr) {
            DEBUGLOG("send message 111111 success");
        });

        client.readMessage("123456789", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
            std::shared_ptr<rocket::TinyPBProtocol> message = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
            DEBUGLOG("msg_id[%s],get response %s", message->m_msg_id.c_str(), message->m_pb_data.c_str());
        });

    });
}

int main() {

    rocket::Config::SetGlobalConfig("../conf/rocket.xml");

    rocket::Logger::InitGlobalLogger();

    // test_connect();

    test_tcp_client();

    return 0;
}