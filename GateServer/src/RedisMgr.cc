#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "Defer.h"
#include "Logger.h"

RedisMgr::RedisMgr(){
    auto cfg=ConfigMgr::GetInstance();
    std::string host=(*cfg)["Redis"]["host"];
    std::string port=(*cfg)["Redis"]["port"];
    std::string passwd=(*cfg)["Redis"]["passwd"];
    std::string poolSize=(*cfg)["Redis"]["poolSize"];
    _pool.reset(new RedisConPool(std::stoi(poolSize),host.c_str(),std::stoi(port.c_str()),passwd.c_str()));
}

RedisMgr::~RedisMgr(){
    Close();
}

void RedisMgr::Close(){
    _pool->Close();
}

bool RedisMgr::Get(const std::string& key, std::string& value){
    auto conn=_pool->getConnection();
    if(conn==nullptr){
        LOG_DEBUG<<"Redis连接获取失败";
        return false;
    }
        
    
    redisReply* reply=(redisReply*)redisCommand(conn->get(),"GET %s",key.c_str());

    if(reply==nullptr){
        LOG_DEBUG<<"redis连接GET获取失败";
        _pool->returnConnection(std::move(conn));
        return false;
    }

    Defer defer([this,&conn,&reply](){
        freeReplyObject(reply);
        _pool->returnConnection(std::move(conn));});

    //GET的key不存在
    if(reply->type!=REDIS_REPLY_STRING){
        LOG_DEBUG<<"redis连接GET key不存在";
        return false;
    }

    value=reply->str;
    return true;
}


bool RedisMgr::Set(const std::string& key, const std::string& value) {
    auto conn = _pool->getConnection();
    if (conn == nullptr){
        LOG_DEBUG<<"Redis连接获取失败";
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(conn->get(),
                            "SET %s %s", key.c_str(), value.c_str());
    if (reply == nullptr){
        LOG_DEBUG<<"redis连接SET失败";
        _pool->returnConnection(std::move(conn));
        return false;
    }

    // SET 成功返回状态回复 "OK"
    bool ok = (reply->type == REDIS_REPLY_STATUS) &&
              (std::string(reply->str) == "OK");
    freeReplyObject(reply);
    _pool->returnConnection(std::move(conn));
    return ok;
}

bool RedisMgr::Del(const std::string& key) {
    auto conn = _pool->getConnection();
    if (conn == nullptr){
        LOG_DEBUG<<"Redis连接获取失败";
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(conn->get(), "DEL %s", key.c_str());
    if (reply == nullptr){
        LOG_DEBUG<<"redis连接DEL失败";
        _pool->returnConnection(std::move(conn));
        return false;
    }

    bool ok = (reply->type == REDIS_REPLY_INTEGER);   // DEL 返回删除的个数
    freeReplyObject(reply);
    _pool->returnConnection(std::move(conn));
    return ok;
}

bool RedisMgr::ExistsKey(const std::string& key) {
    auto conn = _pool->getConnection();
    if (conn == nullptr){
        LOG_DEBUG<<"Redis连接获取失败";
        return false;
    }

    redisReply* reply = (redisReply*)redisCommand(conn->get(), "EXISTS %s", key.c_str());
    if (reply == nullptr){
        LOG_DEBUG<<"redis连接EXISTKey失败";
        _pool->returnConnection(std::move(conn));
        return false;
    }

    // EXISTS 返回整数 1(存在)/ 0(不存在)
    bool exists = (reply->type == REDIS_REPLY_INTEGER) && (reply->integer == 1);
    freeReplyObject(reply);
    _pool->returnConnection(std::move(conn));
    return exists;
}