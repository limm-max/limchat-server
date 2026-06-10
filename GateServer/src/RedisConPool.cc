#include "RedisConPool.h"
#include "Logger.h"

RedisConPool::RedisConPool(int poolSize,const char* host, int port, const char* pwd)
    :_b_stop(false),_poolSize(poolSize),_host(host),_port(port){

        for(int i=0;i<_poolSize;++i){
            
            redisContext* ctx=redisConnect(host,port);
            if(ctx==nullptr || ctx->err){
                LOG_ERROR<<"redis创建连接失败"<<(ctx? std::string(":")+ctx->errstr:"");
                if (ctx) redisFree(ctx);
                continue;

            }

            //如果有密码，进行AUTH认证
            if(pwd!=nullptr && strlen(pwd)>0){
                redisReply* reply=(redisReply*)redisCommand(ctx,"AUTH %s",pwd);
                if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
                    LOG_ERROR<<"redis认证失败";
                    if(reply) freeReplyObject(reply);
                    redisFree(ctx);
                    continue;
                }
                freeReplyObject(reply);
            }

            _connections.push(std::make_unique<RedisConnection>(ctx));
        }
        LOG_DEBUG<<"redis 连接池初始化结束，连接池中的连接数为:"<<_connections.size();
    }


RedisConPool::~RedisConPool(){
    Close();
    std::unique_lock<std::mutex> lock(_mutex);
    while (!_connections.empty()) _connections.pop();

    
}

void RedisConPool::Close(){
    _b_stop=true;
    _cond.notify_all();
}

std::unique_ptr<RedisConnection> RedisConPool::getConnection(){
    std::unique_lock<std::mutex> lock(_mutex);
    //等待条件，lamda是条件逻辑
    _cond.wait(lock,[this]{
        if(_b_stop) return true;
        return !_connections.empty();
    });

    if(_b_stop) return nullptr;
    std::unique_ptr<RedisConnection> conn(std::move(_connections.front()));
    _connections.pop();
    return conn;
}

void RedisConPool::returnConnection(std::unique_ptr<RedisConnection> conn){
    std::unique_lock<std::mutex> lock(_mutex);
    if(_b_stop) return;
    _connections.push(std::move(conn));
    _cond.notify_one();
}