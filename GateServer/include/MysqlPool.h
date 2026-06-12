#pragma once
#include "MysqlConnection.h"
#include<queue>
#include<mutex>
#include<condition_variable>
#include<atomic>
#include<thread>
#include<string>

//池子里包含一个线程，负责进行心跳保活
class MysqlPool{
public:
    //初始化，mysql服务端host、port、账号、密码、schema、池大小
    MysqlPool(const std::string& host,const std::string& port,
                const std::string& user,const std::string& passwd,
                const std::string& schema,int poolSize);
    ~MysqlPool();

    std::unique_ptr<MysqlConnection> getConnection();
    void returnConnection(std::unique_ptr<MysqlConnection> conn);

    void close();

private:
    void checkConnection();
    std::string _url,_user,_pass,_schema;
    int _poolSize;

    std::queue<std::unique_ptr<MysqlConnection>> _pool;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::atomic<bool> _b_stop;
    std::thread _check_thread;

};
