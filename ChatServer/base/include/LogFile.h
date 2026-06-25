#pragma once

#include "Noncopyable.h"
#include "MutexLock.h"

#include <memory>
#include <string>

//---------------
//加上锁、滚动（按照大小和日期定期开新的文件）
//--------------------------
namespace lim {

namespace FileUtil { class AppendFile; }   // 前置声明

class LogFile : Noncopyable {
public:
    LogFile(const std::string& basename,        //用于滚动文件名的前缀
            off_t              rollSize,         // 多大触发滚动(字节)
            bool               threadSafe = true,
            int                flushInterval = 3,// 每隔多少秒 flush 一次
            int                checkEveryN = 1024);// 每写多少次检查一次
    ~LogFile();

    // 追加一条日志
    void append(const char* logline, int len);

    // 强制 flush
    void flush();

    // 强制滚动(测试用)
    bool rollFile();

private:
    void append_unlocked(const char* logline, int len);

    // 生成日志文件名:basename.YYYYmmdd-HHMMSS.hostname.pid.log
    static std::string getLogFileName(const std::string& basename,
                                      time_t* now);

    const std::string basename_;
    const off_t       rollSize_;
    const int         flushInterval_;
    const int         checkEveryN_;

    int               count_;          // 写入次数计数

    std::unique_ptr<MutexLock>             mutex_;
    time_t                                 startOfPeriod_; // 当前文件的起始日期(00:00)
    time_t                                 lastRoll_;      // 上次滚动时间
    time_t                                 lastFlush_;     // 上次 flush 时间
    std::unique_ptr<FileUtil::AppendFile>  file_;          // 真正的文件对象

    const static int kRollPerSeconds_ = 60 * 60 * 24;  // 一天的秒数
};

} // namespace lim