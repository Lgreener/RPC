#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H

#include "rocket/common/config.h"
#include "rocket/common/mutex.h"
#include "rocket/net/timer_event.h"
#include <bits/stdint-intn.h>
#include <memory>
#include <vector>
#include <string>
#include <queue>
#include <semaphore.h>

namespace rocket {

// template <typename... Args>
// std::string formatString(const char *str, Args &&... args) {
//     int size = std::snprintf(nullptr, 0, str, args...);

//     std::string result;
//     if (size > 0) {
//         result.resize(size);
//         std::snprintf(&result[0], size + 1, str, args...);
//     }

//     return result;
// }


// 检查是否有可变参数
template<typename ... Args>
constexpr bool has_format_args() {
    return sizeof...(Args) > 0;
}

// 如果没有格式参数，提供一个重载版本
template <typename... Args>
typename std::enable_if<!has_format_args<Args...>(), std::string>::type
formatString(const char *format) {
    return std::string(format);
}

// 如果有格式参数，使用这个版本
template<typename ... Args>
typename std::enable_if<has_format_args<Args...>(), std::string>::type
formatString(const char* format, Args ... args) {
    int size = std::snprintf(nullptr, 0, format, args ...) + 1; // 获取所需的缓冲区大小
    if (size <= 0) { throw std::runtime_error("Error during formatting."); }
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format, args ...);
    return std::string(buf.get(), buf.get() + size - 1); // 排除末尾的 '\0'
}

// ----------RPC框架日志---------- //
#define DEBUGLOG(str, ...)                                                     \
    if (rocket::Logger::GetGobalLogger()->getLogLevel() <= rocket::Debug) {    \
        rocket::Logger::GetGobalLogger()->pushLog(                             \
            rocket::LogEvent(rocket::LogLevel::Debug).toString() + "[" +       \
            std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +   \
            rocket::formatString(str, ##__VA_ARGS__) + '\n');                  \
    }

#define INFOLOG(str, ...)                                                      \
    if (rocket::Logger::GetGobalLogger()->getLogLevel() <= rocket::Info) {     \
        rocket::Logger::GetGobalLogger()->pushLog(                             \
            rocket::LogEvent(rocket::LogLevel::Info).toString() + "[" +        \
            std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +   \
            rocket::formatString(str, ##__VA_ARGS__) + '\n');                  \
    }

#define ERRORLOG(str, ...)                                                     \
    if (rocket::Logger::GetGobalLogger()->getLogLevel() <= rocket::Error) {    \
        rocket::Logger::GetGobalLogger()->pushLog(                             \
            rocket::LogEvent(rocket::LogLevel::Error).toString() + "[" + "[" + \
            std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +   \
            rocket::formatString(str, ##__VA_ARGS__) + '\n');                  \
    }
// ----------RPC框架日志---------- //

// ----------业务逻辑日志---------- //
#define APPDEBUGLOG(str, ...)                                                     \
    if (rocket::Logger::GetGobalLogger()->getLogLevel() <= rocket::Debug) {    \
        rocket::Logger::GetGobalLogger()->pushAppLog(                             \
            rocket::LogEvent(rocket::LogLevel::Debug).toString() + "[" +       \
            std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +   \
            rocket::formatString(str, ##__VA_ARGS__) + '\n');                  \
    }

#define APPINFOLOG(str, ...)                                                      \
    if (rocket::Logger::GetGobalLogger()->getLogLevel() <= rocket::Info) {     \
        rocket::Logger::GetGobalLogger()->pushAppLog(                             \
            rocket::LogEvent(rocket::LogLevel::Info).toString() + "[" +        \
            std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +   \
            rocket::formatString(str, ##__VA_ARGS__) + '\n');                  \
    }

#define APPERRORLOG(str, ...)                                                     \
    if (rocket::Logger::GetGobalLogger()->getLogLevel() <= rocket::Error) {    \
        rocket::Logger::GetGobalLogger()->pushAppLog(                             \
            rocket::LogEvent(rocket::LogLevel::Error).toString() + "[" + "[" + \
            std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +   \
            rocket::formatString(str, ##__VA_ARGS__) + '\n');                  \
    }
// ----------业务逻辑日志---------- //

enum LogLevel { 
    Unknown = 0, 
    Debug = 1, 
    Info = 2, 
    Error = 3 
};

class AsyncLogger {
public:
    typedef std::shared_ptr<AsyncLogger> s_ptr;
    AsyncLogger(const std::string& file_name,const  std::string& file_path, int file_size);

    static void* Loop(void*);

    void stop();

    // 将文件刷新到磁盘
    void flush();

    void pushLogBuffer(std::vector<std::string>& vec);

public:
    pthread_t m_thread;
    sem_t m_semaphore;

private:
    std::queue<std::vector<std::string>> m_buffer;

    // m_file_path/m_file_name_yyyymmdd.m_no
    std::string m_file_name; // 日志输出文件名字
    std::string m_file_path; // 日志输出路径
    int m_max_file_size {0}; // 日志单个文件最大大小

    pthread_cond_t m_condtion; // 条件变量
    Mutex m_mutex;

    std::string m_date; // 上次打印日志的文件日期
    FILE* m_file_hanlder {NULL}; // 当前打开的日志文件句柄

    bool m_reopen_flag {false}; // 是否重新打开日志

    int m_no {0}; //日志文件序号

    bool m_stop_flag {false};
};



class Logger {
public:
    typedef std::shared_ptr<Logger> s_ptr;

    Logger(LogLevel level, int type = 1);

    void Init();

    void pushLog(const std::string &msg);

    void pushAppLog(const std::string &msg);

    void log();

    LogLevel getLogLevel() const { return m_set_level; }

    void syncLoop();

public:
    static Logger *GetGobalLogger();

    static void InitGlobalLogger(int type = 1);

private:
    LogLevel m_set_level;

    std::vector<std::string> m_buffer;
    std::vector<std::string> m_app_buffer;

    Mutex m_mutex;

    Mutex m_app_mutex;

    AsyncLogger::s_ptr m_asnyc_logger;

    AsyncLogger::s_ptr m_asnyc_app_logger;

    TimerEvent::s_ptr m_timer_event;

    // 1 表示异步日志； 0 表示同步日志
    int m_type {0};

};

std::string LogLevelToString(LogLevel level);

LogLevel StringToLogLevel(const std::string &log_level);

class LogEvent {
public:
    LogEvent(LogLevel level) : m_level(level) {}

    std::string getFileName() const { return m_file_name; }

    LogLevel getLogLevel() const { return m_level; }

    std::string toString();

private:
    std::string m_file_name; //文件名
    int32_t m_file_line;     //行号
    int32_t m_pid;           //进程号
    int32_t m_thread_id;     //线程号

    LogLevel m_level; //日志级别
};


} // namespace rocket

#endif
