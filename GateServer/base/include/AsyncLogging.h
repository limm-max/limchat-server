#pragma once

#include "Noncopyable.h"
#include "MutexLock.h"
#include "Condition.h"
#include "CountDownLatch.h"
#include "Thread.h"
#include "FixedBuffer.h"
#include "LogFile.h"

#include <atomic>
#include <memory>
#include <vector>

namespace lim {

class AsyncLogging : Noncopyable {
public:
    // basename: 日志文件名前缀(传给内部的 LogFile)
    // rollSize: 滚动大小
    // flushInterval: 后端定期唤醒间隔(秒),即使 buffer 没满也要写盘
    AsyncLogging(const std::string& basename,
                 off_t              rollSize,
                 int                flushInterval = 3);

    ~AsyncLogging() {
        if (running_) {
            stop();
        }
    }

    // 前端线程调用:把一条日志塞进 currentBuffer_
    void append(const char* logline, int len);

    // 启动后端线程,主线程调用
    void start() {
        running_ = true;
        thread_.start();
        latch_.wait();   // 等后端线程初始化完成
    }

    // 停止后端线程(优雅退出)，主线程
    void stop() {
        running_ = false;
        cond_.notify();
        thread_.join();
    }

private:
    // 后端线程的入口函数
    void threadFunc();

    using Buffer       = detail::FixedBuffer<detail::kLargeBuffer>;  // 4MB
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr    = BufferVector::value_type;   // = std::unique_ptr<Buffer>

    const int           flushInterval_;
    std::atomic<bool>   running_;
    const std::string   basename_;
    const off_t         rollSize_;
    Thread              thread_;
    CountDownLatch      latch_;     // 主线程等后端启动完成

    MutexLock           mutex_;
    Condition           cond_;      // 前端通知后端"有满 buffer 了"

    BufferPtr           currentBuffer_;
    BufferPtr           nextBuffer_;
    BufferVector        buffers_;   // 待写盘队列
};

} // namespace lim