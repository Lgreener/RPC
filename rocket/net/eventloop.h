#ifndef ROCKET_NET_EVENTLOOP_H
#define ROCKET_NET_EVENTLOOP_H

#include <pthread.h>
#include <set>
#include <functional>
#include <queue>
#include "rocket/common/mutex.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/timer_event.h"
#include "rocket/net/wakeup_fd_event.h"
#include "rocket/net/timer.h"

namespace rocket {
    class EventLoop{
        public:
            EventLoop();

            ~EventLoop();

            void loop();

            void wakeup();

            void stop();

            void addEpollEvent(FdEvent* event);

            void deleteEpollEvent(FdEvent* event);

            bool isInLoopThread();

            void addTask(std::function<void()> cb, bool is_wake_up = false);

            void addTimerEvent(TimerEvent::s_ptr event);

        private:
            void dealWakeup();

            void initWakeUpFdEvent();

            void initTimer();

        private:
            pid_t m_thread_pid {0};

            int m_epoll_fd {0};

            int m_wakeup_fd {0};

            WakeUpFdEvent* m_wakeup_fd_event {NULL};

            bool m_stop_flag {false};

            std::set<int> m_listen_fds;

            std::queue<std::function<void()>> m_pending_tasks;

            Mutex m_mutex;

            Timer* m_timer {NULL};

    };
}


#endif