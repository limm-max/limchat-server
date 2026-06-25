// IOServicePool.h
#pragma once

#include <vector>
#include <thread>
#include "Singleton.h"
#include"const.h"


//io_context池，一个io_context一个线程
//guard：确保io_context.run() 在没任务时空闲也不退出
class IOServicePool:public Singleton<IOServicePool> {
    friend class Singleton<IOServicePool>;

public:
    using IOService=net::io_context;
    using Work=net::executor_work_guard<IOService::executor_type>;
    using WorkPtr=std::unique_ptr<Work>;

    ~IOServicePool();
    IOService& GetIOService();
    void stop();

private:
    IOServicePool(std::size_t size=std::thread::hardware_concurrency());
    std::vector<IOService> _ioServices;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _threads;
    std::size_t _nextIOService;
};