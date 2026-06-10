#include "LogFile.h"
#include "FileUtil.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

namespace lim {

LogFile::LogFile(const std::string& basename,
                 off_t              rollSize,
                 bool               threadSafe,
                 int                flushInterval,
                 int                checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0),
      mutex_(threadSafe ? new MutexLock : nullptr),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0) {
    rollFile();   // 初始就滚动一次,创建第一个日志文件
}

LogFile::~LogFile() = default;

void LogFile::append(const char* logline, int len) {
    if (mutex_) {
        MutexLockGuard lock(*mutex_);
        append_unlocked(logline, len);
    } else {
        append_unlocked(logline, len);
    }
}

void LogFile::flush() {
    if (mutex_) {
        MutexLockGuard lock(*mutex_);
        file_->flush();
    } else {
        file_->flush();
    }
}

void LogFile::append_unlocked(const char* logline, int len) {
    file_->append(logline, len);

    // 1. 文件太大,触发滚动
    if (file_->writtenBytes() > rollSize_) {
        rollFile();
        return;
    }

    // 2. 每写 checkEveryN_ 次,检查是否要按日滚动 / 定期 flush
    ++count_;
    if (count_ >= checkEveryN_) {
        count_ = 0;
        time_t now = ::time(nullptr);
        time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
        // 跨天了
        if (thisPeriod != startOfPeriod_) {
            rollFile();
        } else if (now - lastFlush_ > flushInterval_) {
            // 没跨天但到了 flush 时间
            lastFlush_ = now;
            file_->flush();
        }
    }
}

bool LogFile::rollFile() {
    time_t now = 0;
    std::string filename = getLogFileName(basename_, &now);

    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_) {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_.reset(new FileUtil::AppendFile(filename));   // 创建新文件
        return true;
    }
    return false;
}

std::string LogFile::getLogFileName(const std::string& basename, time_t* now) {
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = ::time(nullptr);
    ::localtime_r(now, &tm);   // 线程安全的 localtime
    ::strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S", &tm);
    filename += timebuf;

    filename += ".log";
    return filename;
}

} // namespace lim