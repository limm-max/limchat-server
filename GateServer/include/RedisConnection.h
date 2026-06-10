#pragma once
#include <hiredis/hiredis.h>

class RedisConnection{
public:
    explicit RedisConnection(redisContext* ctx): _ctx(ctx) {}

    ~RedisConnection(){
        if(_ctx){
            redisFree(_ctx);
        }
    }

    RedisConnection(const RedisConnection&) = delete;
    RedisConnection& operator=(const RedisConnection&) = delete;

    redisContext* get() const { return _ctx; }   

private:
    redisContext* _ctx;
};