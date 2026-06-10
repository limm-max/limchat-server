// ============================================================
// Timestamp.cc
// 实现 Timestamp 类的成员函数
// ============================================================

#include "Timestamp.h"

#include <sys/time.h>   // gettimeofday
#include <cstdio>       // snprintf
#include <ctime>        // localtime_r

namespace lim {

    //获取现在的时间戳
Timestamp Timestamp::now() {
    struct timeval tv;
    // gettimeofday: Linux 系统调用，获取当前时间（精度到微秒）
    gettimeofday(&tv, nullptr);
    
    int64_t seconds = tv.tv_sec;
    int64_t microseconds = seconds * kMicroSecondsPerSecond + tv.tv_usec;
    return Timestamp(microseconds);
}

std::string Timestamp::toString() const {
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    
    // 格式："秒数.微秒数"，例如 "1778155200.123456"
    snprintf(buf, sizeof(buf), "%ld.%06ld", seconds, microseconds);
    return buf;
}

std::string Timestamp::toFormattedString(bool showMicroseconds) const {
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(
        microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    
    struct tm tm_time;
    // localtime_r: localtime 的线程安全版本
    localtime_r(&seconds, &tm_time);
    
    if (showMicroseconds) {
        int microseconds = static_cast<int>(
            microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), 
                 "%4d-%02d-%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec,
                 microseconds);
    } else {
        snprintf(buf, sizeof(buf),
                 "%4d-%02d-%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900,
                 tm_time.tm_mon + 1,
                 tm_time.tm_mday,
                 tm_time.tm_hour,
                 tm_time.tm_min,
                 tm_time.tm_sec);
    }
    
    return buf;
}

} // namespace lim