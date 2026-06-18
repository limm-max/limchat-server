#pragma once
#include "Singleton.h"
#include "RedisConPool.h"
#include<memory>
#include<string>

class RedisMgr:public Singleton<RedisMgr>{
    friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();

    //后续涉及到的操作再加
    bool Get(const std::string& key,std::string& value);
    bool Set(const std::string& key,const std::string& value);
    bool Del(const std::string& key);
    bool ExistsKey(const std::string& key);
    void Close();

private:
    RedisMgr();
    std::unique_ptr<RedisConPool> _pool;
};