#ifndef ROCKET_NET_IO_THREAD_GROUP_H
#define ROCKET_NET_IO_THREAD_GROUP_H

#include "rocket/common/log.h"
#include "rocket/net/io_thread.h"
#include <vector>

namespace rocket {
class IOThreadGroup {
public:
    IOThreadGroup(int size);
    ~IOThreadGroup();

    void start();
    void join();
    IOThread *getIOThread(); //从线程组中获取一个可用的IO线程

private:
    int m_size{0};
    std::vector<IOThread *> m_io_thread_groups;

    int m_index{0};
};

} // namespace rocket

#endif