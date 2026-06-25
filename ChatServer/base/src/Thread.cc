#include "Thread.h"
#include "CurrentThread.h"

#include<semaphore.h>
#include<stdio.h>

namespace lim{

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func,const std::string& name)
    :started_(false),
    joined_(false),
    tid_(0),
    func_(std::move(func)), //移动语义，避免拷贝
    name_(name){
        setDefaultName();
    }

Thread::~Thread(){
    // 析构时如果线程已经启动且没有 join 过,就 detach
    // detach 让线程自生自灭(脱离 Thread 对象,自己跑完自己释放)
    // 否则 std::thread 析构时若仍 joinable 会调 std::terminate 直接挂掉程序!
    if(started_ && !joined_){
        thread_->detach();
    }
}

void Thread::start(){
    started_=true;

    sem_t sem;
    sem_init(&sem,false,0);
    //子进程的入口函数要访问外部变量，需要使用lambda的[&] 捕获语法来访问
    thread_=std::shared_ptr<std::thread>(new std::thread([&](){
        // ===== 这里开始已经是新线程在跑了 =====
        tid_=CurrentThread::tid();
        sem_post(&sem);
        func_();
    }));

    // 主线程在这里阻塞等子线程发信号
    // 等到 sem_post 后才返回,保证 tid_ 已经设置好
    sem_wait(&sem);
    sem_destroy(&sem);
}

void Thread::join(){
    joined_=true;
    thread_->join();
}

void Thread::setDefaultName(){
    int num=++numCreated_;
    if(name_.empty()){
        char buf[32]={0};
        snprintf(buf,sizeof(buf),"Thread%d",num);
        name_=buf;
    }
}


} //namespace lim