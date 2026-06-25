#include "IOServicePool.h"
#include"Logger.h"

IOServicePool::IOServicePool(std::size_t size)
    :_ioServices(size),_works(size),_nextIOService(0){
        for(std::size_t i=0;i<size;++i){
            _works[i]=std::make_unique<Work>(net::make_work_guard(_ioServices[i]));
        }

        for (std::size_t i = 0; i < size; ++i) {
            _threads.emplace_back([this, i]() { _ioServices[i].run(); });
        }
}

IOServicePool::~IOServicePool() { stop(); }

boost::asio::io_context& IOServicePool::GetIOService() {
    auto& service = _ioServices[_nextIOService++];
    if (_nextIOService == _ioServices.size()) _nextIOService = 0;
    return service;
}

void IOServicePool::stop() {
    for (auto& work : _works) work.reset();      // 释放 work guard，run() 才能返回
    for (auto& t : _threads) if (t.joinable()) t.join();
}