#include "Logger.h"
#include "CurrentThread.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

namespace lim {

// ============================================================
// 全局变量
// ============================================================

// 默认日志级别 INFO(可在 main 里 setLogLevel 改)
Logger::LogLevel g_logLevel = Logger::INFO;

// 默认输出函数:写到 stdout
static void defaultOutput(const char* msg, int len) {
    ::fwrite(msg, 1, len, stdout);
}

// 默认 flush 函数:flush stdout
static void defaultFlush() {
    ::fflush(stdout);
}

// 全局函数指针(Day 7 异步化时会被替换!)
static Logger::OutputFunc g_output = defaultOutput;
static Logger::FlushFunc  g_flush  = defaultFlush;

// ============================================================
// 日志级别字符串(末尾两空格保证对齐)
// ============================================================
const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

// ============================================================
// Impl 实现
// ============================================================

Logger::Impl::Impl(LogLevel level, const char* file, int line)
    : time_(Timestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(nullptr) {
    // 提取文件名(去掉路径)
    const char* slash = strrchr(file, '/');
    basename_ = (slash ? slash + 1 : file);

    // 写入"时间 线程ID 级别"
    formatTime();
    stream_ << CurrentThread::tid() << " ";
    stream_ << LogLevelName[level];
}

void Logger::Impl::formatTime() {
    int64_t microSeconds = time_.microSecondsSinceEpoch();
    time_t  seconds = static_cast<time_t>(microSeconds / Timestamp::kMicroSecondsPerSecond);
    int     micro   = static_cast<int>(microSeconds % Timestamp::kMicroSecondsPerSecond);

    struct tm tm_time;
    ::localtime_r(&seconds, &tm_time);

    char buf[64] = {0};
    snprintf(buf, sizeof buf, "%4d%02d%02d %02d:%02d:%02d.%06d ",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
             micro);
    stream_ << buf;
}

void Logger::Impl::finish() {
    stream_ << " - " << basename_ << ":" << line_ << "\n";
}

// ============================================================
// Logger 实现
// ============================================================

Logger::Logger(const char* file, int line, LogLevel level)
    : impl_(level, file, line) {
}

Logger::Logger(const char* file, int line, LogLevel level, const char* func)
    : impl_(level, file, line) {
    impl_.stream_ << func << " ";
}

Logger::~Logger() {
    impl_.finish();   // 加 " - file:line\n"
    const LogStream::Buffer& buf = stream().buffer();
    g_output(buf.data(), buf.length());

    // FATAL 级别:flush + 终止程序!
    if (impl_.level_ == FATAL) {
        g_flush();
        ::abort();
    }
}

void Logger::setLogLevel(LogLevel level) {
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out) {
    g_output = out;
}

void Logger::setFlush(FlushFunc flush) {
    g_flush = flush;
}

} // namespace lim