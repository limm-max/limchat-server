#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <hiredis/hiredis.h> 
#include "RedisConnection.h"

class RedisConPool{
public:
    RedisConPool(int poolSize,const char* host,int port,const char* pwd);
    ~RedisConPool();

    //借还连接
    std::unique_ptr<RedisConnection> getConnection();
    void returnConnection(std::unique_ptr<RedisConnection> conn);

    //关闭连接池
    void Close(){

    }

private:
    std::atomic<bool> _b_stop;  //停机标志
    int _poolSize;
    std::string _host;
    int _port;
    std::queue<std::unique_ptr<RedisConnection>> _connections;
    std::mutex _mutex;
    std::condition_variable _cond;
};