#pragma once

#include "Condition.h"
#include "MutexLock.h"
#include "Noncopyable.h"

namespace lim{
   
// CountDownLatch:倒计数门闩
// 主线程在 wait 上阻塞,直到 count_ 减到 0 才放行
// 典型用途:主线程等所有工作线程都准备好,再统一启动
class CountDownLatch:Noncopyable{
public:
    explicit CountDownLatch(int count);

    void wait();
    void countDown();
    int getCount() const;

private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};

}//namespace lim;