#pragma once
#include "Singleton.h"
#include "const.h"
#include <functional>
#include <map>

class HttpConnection;

typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem:public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    
    //不同方法的路由分发
    bool HandleGet(std::string path, std::shared_ptr<HttpConnection> con);
    bool HandlePost(std::string path, std::shared_ptr<HttpConnection> con);

    //handler注册
    void RegGet(std::string url, HttpHandler handler);
    void RegPost(std::string url, HttpHandler handler);

private:
    LogicSystem();                                   // 私有构造，只能 GetInstance 造

    //长逻辑业务处理函数
    void HandleUserRegister(std::shared_ptr<HttpConnection> conn); 
    void HandleUserLogin(std::shared_ptr<HttpConnection> conn);
    std::map<std::string, HttpHandler> _get_handlers;  // GET 路由表
    std::map<std::string, HttpHandler> _post_handlers; // POST 路由表
};
