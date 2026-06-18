//-----------------------------------
//封装 pthread_create，构造和启动分离，可以传 std::function<void()> 当任务。
//----------------------------------









#pragma once

#include "Noncopyable.h"


#include<functional>
#include<memory>
#include<string>
#include<thread>
#include<atomic>
#include<unistd.h>

namespace lim
{

class Thread:Noncopyable{
public:
    // 线程要执行的函数类型:无参无返回值
    //std::function<void()>通用函数包装器
    using ThreadFunc=std::function<void()>;

    // explicit 防止 Thread t = someFunc; 这种隐式转换
    explicit Thread(ThreadFunc func,const std::string& name=std::string());
    ~Thread();

    void start();
    void join();

    bool started() const{ return started_;}
    pid_t tid() const { return tid_;}
    const std::string& name() const { return name_; }
    // 静态变量:全进程已创建的 Thread 数量
    // 注意:atomic_int 保证多线程同时 new Thread 时计数器不冲突
    static int numCreated(){return numCreated_.load();}


private:
    void setDefaultName();  // 没传名字时,自动生成 "Thread1"、"Thread2"...

    bool started_;  //启动状态
    bool joined_;   //分离状态
    std::shared_ptr<std::thread> thread_;   //要封装的线程
    pid_t tid_; //线程线程号
    ThreadFunc func_;   //线程入口函数
    std::string name_;  //线程
    static std::atomic_int numCreated_; //使用原子操作库保证变量修改

};
    




} // namespace lim


