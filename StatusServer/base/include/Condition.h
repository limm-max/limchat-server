#pragma once

#include "MutexLock.h"
#include "Noncopyable.h"

#include<pthread.h>

namespace lim{


class Condition:Noncopyable{
public:
    explicit Condition(MutexLock& mutex):mutex_(mutex){
        pthread_cond_init(&cond_,nullptr);
    }

    ~Condition(){
        pthread_cond_destroy(&cond_);
    }

    // 等待条件:必须在持有锁的状态下调用
    // wait 内部:1.释放锁 2.阻塞 3.被唤醒后重新加锁
    void wait();

    bool waitForSeconds(double seconds);

    //唤醒等待队列的线程
    void notify(){
        pthread_cond_signal(&cond_);
    }

    void notifyAll(){
        pthread_cond_broadcast(&cond_);
    }




private:
    MutexLock& mutex_;
    pthread_cond_t cond_;





};

}//namespace lim