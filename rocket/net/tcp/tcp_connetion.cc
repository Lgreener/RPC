#include <memory>
#include <unistd.h>
#include <utility>

#include "rocket/common/log.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/net/rpc/rpc_dispatcher.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/tinypb_coder.h"

namespace rocket {

TcpConnection::TcpConnection(EventLoop* event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr, NetAddr::s_ptr local_addr, TcpConnectionType type)
    : m_event_loop(event_loop), m_local_addr(local_addr), m_peer_addr(peer_addr), m_state(NotConnected), m_fd(fd) , m_connection_type(type) {

    m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);

    m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

    m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(fd);

    m_fd_event->setNonBlock();

    m_coder = new TinyPBCoder();

    if (m_connection_type == TcpConnectionByServer) {
        listenRead();
    }

}

TcpConnection::~TcpConnection() {
    DEBUGLOG("~TcpConnection");
    if (m_coder) {
        delete m_coder;
        m_coder = NULL;
    }
}

void TcpConnection::onRead() {
    // 1.从 socket 缓冲区，调用系统的 read 函数读取字节, 写入 in_buffer 里面
    if (m_state != Connected) {
        ERRORLOG("onRead error, client has already disconneced, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
        return;
    }
    bool is_read_all = false;
    bool is_close = false;
    while (!is_read_all) {
        if (m_in_buffer->writeAble() == 0) {
            m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());
        }
        int read_count = m_in_buffer->writeAble();
        int write_index = m_in_buffer->writeIndex();
        int rt = read(m_fd, &(m_in_buffer->m_buffer[write_index]), read_count);
        INFOLOG("success read %d bytes from addr[%s],client fd[%d]", rt, m_peer_addr->toString().c_str(), m_fd);
        if (rt > 0) {
            m_in_buffer->moveWriteIndex(rt);
            if (rt == read_count) {
                continue;
            } else if (rt < read_count) {
                is_read_all = true;
                break;
            }
        } else if (rt == 0) {
            is_close = true; // 对方关闭连接,读取到了 EOF（文件结束标志）
            break;
        }else if (rt == -1 && errno == EAGAIN) {
            is_read_all = true; // 数据读完了
            break;
        }else {
            is_close = true; // 发生错误，关闭连接
            break;
        }
    }

    if (is_close) {
        // 处理关闭连接
        INFOLOG("peer closed, peer addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
        clear();
        return;
    }

    if (!is_read_all) {
        ERRORLOG("not read all data");
    }

    // RPC协议解析
    TcpConnection::excute();
}

void TcpConnection::excute() {

    if (m_connection_type == TcpConnectionByServer) {
        // 2. 将RPC请求执行业务逻辑，获取RPC响应，再把RPC响应发送回去
        std::vector<AbstractProtocol::s_ptr> result;
        std::vector<AbstractProtocol::s_ptr> replay_messages;
        m_coder->decode(result, m_in_buffer);

        for (size_t i = 0; i < result.size(); ++i) {
            // 1.针对每一个请求，调用 rpc 方法，获取响应 message
            // 2.将响应message放入到发送缓冲区，监听可写事件回包
            INFOLOG("success get request[%s] from client[%s]", result[i]->m_msg_id.c_str(), m_peer_addr->toString().c_str());
            std::shared_ptr<TinyPBProtocol> message = std::make_shared<TinyPBProtocol>();

            RpcDispatcher::GetRpcDispatcher()->dispatch(result[i], message, this);

            replay_messages.emplace_back(message);
        }

        m_coder->encode(replay_messages, m_out_buffer);
        listenWrite();

    } else {
        
        //从buffer里decode得到message对象，执行其回调
        std::vector<AbstractProtocol::s_ptr> result;
        m_coder->decode(result, m_in_buffer);

        for (size_t i = 0; i < result.size(); ++i) {
            std::string msg_id = result[i]->m_msg_id;
            auto it = m_read_dones.find(msg_id);
            if (it != m_read_dones.end()) {
                it->second(result[i]);// 里面构造的对象传到外面的回调函数中
            }
        }
    }
}

void TcpConnection::onWrite() {
    // 3.将当前out_buffer里面的数据全部发送给client
    if (m_state != Connected) {
        ERRORLOG("onwrite error,client has already disconneced, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
        return;
    }

    if (m_connection_type == TcpConnectionByClient){
        // 1. 将message 编码得到字节流
        // 2. 将字节流写入到buffer里面，然后全部发送

        std::vector<AbstractProtocol::s_ptr> messages;

        for (size_t i = 0; i < m_write_dones.size(); ++i) {
            messages.push_back(m_write_dones[i].first);
        }

        m_coder->encode(messages, m_out_buffer);

    }

    bool is_write_all = false;
    while (true) {
        if (m_out_buffer->readAble() == 0) {
            DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
            is_write_all = true;
            break;
        }
        int write_size = m_out_buffer->readAble();
        int read_index = m_out_buffer->readIndex();
        int rt = write(m_fd, &(m_out_buffer->m_buffer[read_index]), write_size);

        if (rt >= write_size) {
            DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
            is_write_all = true;
            break;
        } else if (rt == -1 && errno == EAGAIN) {
            //发送缓冲区已满，不能再发送了。
            //这种情况我们等下次fd可写的时候再次发送数据即可

            ERRORLOG("write data error, errno==EAGIN and rt == -1");
            break;
        }
    }
    
    if (is_write_all) {
        m_fd_event->cancle(FdEvent::OUT_EVENT);
        m_event_loop->addEpollEvent(m_fd_event);
    }

    if(m_connection_type == TcpConnectionByClient) {
        for (size_t i = 0; i < m_write_dones.size(); ++i) {
            m_write_dones[i].second(m_write_dones[i].first);
        }

        m_write_dones.clear();
    }
}

void TcpConnection::setState(const TcpState state) { 
    m_state = Connected; 
}

TcpState TcpConnection::getState() { 
    return m_state; 
}

void TcpConnection::clear() {
    //处理一些关闭连接后的清理动作
    if (m_state == Closed) {
        return;
    }
    m_fd_event->cancle(FdEvent::IN_EVENT);
    m_fd_event->cancle(FdEvent::OUT_EVENT);
    
    INFOLOG("Connection closed, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
    m_event_loop->deleteEpollEvent(m_fd_event);
    m_state = Closed;
}

void TcpConnection::shutdown() {
    if (m_state == Closed || m_state == NotConnected) {
        return;
    }
    //处于半关闭
    m_state = HalfClosing;

    //调用shutdown关闭读写，意味着服务器不会再对这个fd进行读写操作了
    //发送FIN报文，触发了四次挥手的第一个阶段
    //当fd发生可读事件，但是可读的数据为0，即对端发送了FIN
    ::shutdown(m_fd, SHUT_RDWR);
}

void TcpConnection::setConnectionType(TcpConnectionType type) { 
    m_connection_type = type;
}

void TcpConnection::listenWrite() {
    m_fd_event->listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));
    m_event_loop->addEpollEvent(m_fd_event);
}

void TcpConnection::listenRead() {
    m_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::onRead, this));
    m_event_loop->addEpollEvent(m_fd_event);
}

void TcpConnection::pushSendMessage(AbstractProtocol::s_ptr messages, std::function<void(AbstractProtocol::s_ptr)> done){
    m_write_dones.push_back(std::make_pair(messages, done));
}

void TcpConnection::pushReadMessage( const std::string &msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
    m_read_dones.insert(std::make_pair(msg_id, done));
}

NetAddr::s_ptr TcpConnection::getLocalAddr() { return m_local_addr; }

NetAddr::s_ptr TcpConnection::getPeerAddr() { return m_peer_addr; }

} // namespace rocket