#pragma once

#include "Noncopyable.h"
#include "CurrentThread.h"

#include<pthread.h>
#include<assert.h>

// ============================================================
// MutexLock:对 pthread_mutex_t 的 OOP 封装
// 提供基本的 lock/unlock 接口 + 调试用的"持有者检查"
// ============================================================

namespace lim{

class MutexLock:Noncopyable{

public:
    MutexLock():holder_(0){
        pthread_mutex_init(&mutex_,nullptr);
    }
    ~MutexLock(){
        assert(holder_==0);
        pthread_mutex_destroy(&mutex_);
    }

    bool isLockedByThisThread() const{
        return holder_==CurrentThread::tid();
    }

    // 断言"当前线程必须持有这把锁",否则崩溃
    // 用于检查"修改这个数据的人有没有先加锁"
    void assertLocked() const{
        assert(isLockedByThisThread());
    }

    //P 加锁
    void lock(){
        pthread_mutex_lock(&mutex_);
        holder_=CurrentThread::tid();
    }

    //V 解锁
    void unlock(){
        holder_=0;
        pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t* getPthreadMutex(){
        return &mutex_;
    }



private:
    friend class Condition;

    pthread_mutex_t mutex_;
    pid_t holder_;  // 当前持有锁的线程 tid,0 表示无人持有



};

// ============================================================
// MutexLockGuard:RAII 风格的锁守卫
// 构造时加锁,析构时自动解锁——再也不会忘记 unlock!
//使用guard，那么即使边界代码抛出异常，也可以在离开作用域的时候guard自动析构解锁
// ============================================================

class MutexLockGuard:Noncopyable{
public:
    explicit MutexLockGuard(MutexLock& mutex):mutex_(mutex){
        mutex_.lock();
    }

    ~MutexLockGuard(){
        mutex_.unlock();
    }


private:
    MutexLock& mutex_;
};


} //namespcace lim

//防呆，避免漏了变量名而没有上锁
#define MutexLockGuard(x) static_assert(false, "Missing guard variable name")