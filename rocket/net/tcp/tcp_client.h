#ifndef ROCKET_NET_TCP_TCP_CLIENT_H
#define ROCKET_NET_TCP_TCP_CLIENT_H

#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/timer_event.h"

#include <functional>
#include <memory>

namespace rocket {

class TcpClient {

public:
    typedef std::shared_ptr<TcpClient>s_ptr;

    TcpClient(NetAddr::s_ptr peer_addr);
    ~TcpClient();

    // 异步的进行 connect
    // 如果connect完成， done会被执行
    void connect(std::function<void()> done);

    // 异步的发送信息
    //如果发送message成功，会调用done函数，函数的入参就是message对象
    void writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);

    // 异步的读取信息
    //如果读取message成功，会调用done函数，函数的入参就是message对象
    void readMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done);

    // 客户端结束loop循环
    void stop();

    int getConnectErrorCode();
    std::string getConnectErrorInfo();

    NetAddr::s_ptr getPeerAddr();
    NetAddr::s_ptr getLocalAddr();

    void initLocalAddr();

    void addTimerEvent(TimerEvent::s_ptr timer_event);

private:
    NetAddr::s_ptr m_peer_addr;
    NetAddr::s_ptr m_local_addr;

    EventLoop *m_event_loop {NULL};

    int m_fd {-1};
    FdEvent* m_fd_event {NULL};

    TcpConnection::s_ptr m_connection;

    // 连接失败的错误码和错误信息
    int m_connect_error_code{0};
    std::string m_connect_error_info;

};

} // namespace rocket
#endif