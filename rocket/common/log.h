#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H

#include "config.h"
#include "mutex.h"
#include <bits/stdint-intn.h>
#include <memory>
#include <queue>
#include <string>

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


#define DEBUGLOG(str, ...)                                                     \
    if (rocket::Logger::GetGobalLogger()->getLogLevel() <= rocket::Debug) {    \
        rocket::Logger::GetGobalLogger()->pushLog(                             \
            rocket::LogEvent(rocket::LogLevel::Debug).toString() + "[" +       \
            std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +   \
            rocket::formatString(str, ##__VA_ARGS__) + '\n');                  \
        rocket::Logger::GetGobalLogger()->log();                               \
    }

#define INFOLOG(str, ...)                                                      \
    if (rocket::Logger::GetGobalLogger()->getLogLevel() <= rocket::Info) {     \
        rocket::Logger::GetGobalLogger()->pushLog(                             \
            rocket::LogEvent(rocket::LogLevel::Info).toString() + "[" +        \
            std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +   \
            rocket::formatString(str, ##__VA_ARGS__) + '\n');                  \
        rocket::Logger::GetGobalLogger()->log();                               \
    }

#define ERRORLOG(str, ...)                                                     \
    if (rocket::Logger::GetGobalLogger()->getLogLevel() <= rocket::Error) {    \
        rocket::Logger::GetGobalLogger()->pushLog(                             \
            rocket::LogEvent(rocket::LogLevel::Error).toString() + "[" + "[" + \
            std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +   \
            rocket::formatString(str, ##__VA_ARGS__) + '\n');                  \
        rocket::Logger::GetGobalLogger()->log();                               \
    }

enum LogLevel { Unknown = 0, Debug = 1, Info = 2, Error = 3 };

class Logger {
public:
    typedef std::shared_ptr<Logger> s_ptr;

    Logger(LogLevel level) : m_set_level(level) {}

    void pushLog(const std::string &msg);

    void log();

    LogLevel getLogLevel() const { return m_set_level; }

public:
    static Logger *GetGobalLogger();

    static void InitGlobalLogger();

private:
    LogLevel m_set_level;

    std::queue<std::string> m_buffer;

    Mutex m_mutex;
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
