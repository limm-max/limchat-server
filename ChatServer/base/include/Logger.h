#pragma once

#include "LogStream.h"
#include "Timestamp.h"

#include <functional>

namespace lim {

class Logger {
public:
    // 日志级别(从低到高)
    //枚举类型在类中，称为类的静态成员
    enum LogLevel {
        TRACE,   // 最详细,通常关闭
        DEBUG,   // 调试用
        INFO,    // 一般信息
        WARN,    // 警告
        ERROR,   // 错误(可恢复)
        FATAL,   // 致命(程序无法继续)
        NUM_LOG_LEVELS,
    };

    // 输出函数类型(关键!Day 7 替换它就异步化了)
    typedef std::function<void(const char* msg, int len)> OutputFunc;
    typedef std::function<void()>                          FlushFunc;

    // 构造:LOG_XXX 宏会调这个
    Logger(const char* file, int line, LogLevel level);
    Logger(const char* file, int line, LogLevel level, const char* func);
    ~Logger();

    LogStream& stream() { return impl_.stream_; }

    // 全局配置
    static LogLevel logLevel();
    static void     setLogLevel(LogLevel level);
    static void     setOutput(OutputFunc);
    static void     setFlush(FlushFunc);

private:
    // Impl:把所有"实现细节"封装在这,Logger 本身只暴露最少接口
    class Impl {
    public:
        Impl(LogLevel level, const char* file, int line);
        void formatTime();    // 格式化时间戳
        void finish();        // 加 " - file:line\n"

        Timestamp time_;        //时间戳
        LogStream stream_;      //日志的格式化引擎
        LogLevel  level_;       //日志级别
        int       line_;        //调用处行号
        const char* basename_;  //调用处文件名
    };

    Impl impl_;
};

// 全局当前日志级别(extern,实际定义在 .cc)
extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel() {
    return g_logLevel;
}

// ============================================================
// LOG_XXX 宏:用户的入口
// ============================================================
// 工作原理:
//   1. 构造一个临时 Logger 对象
//   2. 取出它内部的 stream() 引用
//   3. 用户的 << 操作写到 stream
//   4. 分号结束 → 临时 Logger 析构 → 自动追加换行 + 调用输出函数
//__FILE__ / __LINE__ / __func__ —— 编译器内置宏
//__FILE__当前文件路径,__LINE__当前行号,__func__
// ============================================================

#define LOG_TRACE if (lim::Logger::logLevel() <= lim::Logger::TRACE) \
    lim::Logger(__FILE__, __LINE__, lim::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (lim::Logger::logLevel() <= lim::Logger::DEBUG) \
    lim::Logger(__FILE__, __LINE__, lim::Logger::DEBUG, __func__).stream()
#define LOG_INFO  if (lim::Logger::logLevel() <= lim::Logger::INFO)  \
    lim::Logger(__FILE__, __LINE__, lim::Logger::INFO).stream()
#define LOG_WARN  lim::Logger(__FILE__, __LINE__, lim::Logger::WARN).stream()
#define LOG_ERROR lim::Logger(__FILE__, __LINE__, lim::Logger::ERROR).stream()
#define LOG_FATAL lim::Logger(__FILE__, __LINE__, lim::Logger::FATAL).stream()

} // namespace lim