#include "AsyncLogging.h"
#include "Timestamp.h"

#include <stdio.h>

namespace lim {

AsyncLogging::AsyncLogging(const std::string& basename,
                           off_t              rollSize,
                           int                flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "AsyncLogging"),
      latch_(1),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_() {
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);   // 预留空间,避免频繁扩容
}

// ============================================================
// 前端 append:业务线程调用,目标:尽快返回!
// ============================================================
void AsyncLogging::append(const char* logline, int len) {
    MutexLockGuard lock(mutex_);

    if (currentBuffer_->avail() > len) {
        // 情况 1:当前 buffer 还装得下,直接写
        currentBuffer_->append(logline, len);
    } else {
        // 情况 2:当前 buffer 满了,需要切换
        buffers_.push_back(std::move(currentBuffer_));

        if (nextBuffer_) {
            // 备胎在,顶上!
            currentBuffer_ = std::move(nextBuffer_);
        } else {
            // 极少见:连备胎都没了(后端来不及补)
            // 兜底:新建一个临时 buffer
            currentBuffer_.reset(new Buffer);
        }

        currentBuffer_->append(logline, len);
        cond_.notify();   // ⭐ 唤醒后端线程:有满 buffer 了!
    }
}

// ============================================================
// 后端 threadFunc:独立线程跑,负责把 buffer 写盘
// ============================================================
void AsyncLogging::threadFunc() {
    latch_.countDown();   // 通知主线程"我准备好了"

    // 创建后端真正的输出目标:LogFile
    LogFile output(basename_, rollSize_, false);   // threadSafe=false (后端独占)

    // 后端自己准备 2 个备胎 buffer,等会拿去补位
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();

    BufferVector buffersToWrite;     // 后端独占的局部队列(无锁)
    buffersToWrite.reserve(16);

    while (running_) {
        // ===== 阶段 1:加锁,跟前端交换 buffer =====
        {
            MutexLockGuard lock(mutex_);

            // 队列空:等通知或超时
            if (buffers_.empty()) {
                cond_.waitForSeconds(flushInterval_);
            }

            // 把当前 currentBuffer_ 也强制入队(不管满没满)
            buffers_.push_back(std::move(currentBuffer_));

            // 用 newBuffer1 补上 currentBuffer_,让前端立刻能写
            currentBuffer_ = std::move(newBuffer1);

            // 把整个待写队列抢走(局部变量,后续无锁处理)
            buffersToWrite.swap(buffers_);

            // 如果 nextBuffer_ 空,也补上
            if (!nextBuffer_) {
                nextBuffer_ = std::move(newBuffer2);
            }
        }
        // ===== 释放锁 =====

        // ===== 阶段 2:内存暴涨防护(高并发兜底) =====
        if (buffersToWrite.size() > 25) {
            char buf[256];
            snprintf(buf, sizeof buf,
                     "Dropped log messages at %s, %zd larger buffers\n",
                     Timestamp::now().toFormattedString().c_str(),
                     buffersToWrite.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            // 只保留前 2 个,其他丢掉
            buffersToWrite.erase(buffersToWrite.begin() + 2,
                                 buffersToWrite.end());
        }

        // ===== 阶段 3:慢慢写盘(无锁!不影响前端) =====
        for (const auto& buffer : buffersToWrite) {
            output.append(buffer->data(), buffer->length());
        }

        // 多余的 buffer 留 2 个(其他释放),变成下一轮的备胎
        if (buffersToWrite.size() > 2) {
            buffersToWrite.resize(2);
        }

        // ===== 阶段 4:把写过的 buffer 清空,准备下一轮做备胎 =====
        if (!newBuffer1) {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();    // ⭐ 清空内容,准备复用
        }

        if (!newBuffer2) {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }

    // 退出前最后一次 flush
    output.flush();
}

} // namespace lim