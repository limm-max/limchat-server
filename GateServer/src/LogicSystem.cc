#include "LogicSystem.h"
#include "HttpConnection.h" 
#include "VerifyGrpcClient.h" 
#include "const.h"
#include "Logger.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
LogicSystem::LogicSystem(){
    RegPost("/get_verifycode",[](std::shared_ptr<HttpConnection> connection){

        //1.获取请求体，json
        auto body_str=beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;

        connection->_response.set(http::field::content_type,"text/json");

        //2.json解析body
        json root;   //response的body
        json src_root=json::parse(body_str,nullptr,false);
        if(src_root.is_discarded() || !src_root.contains("email")){
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"]=ErrorCodes::Error_Json;
            beast::ostream(connection->_response.body()) << root.dump();
            return;
        }
        std::string email=src_root.value("email","");

        //3.解析成功则转向vatify
        GetVerifyRsp rsp=VerifyGrpcClient::GetInstance()->GetVerifyCode(email);
        root["error"]=rsp.error();
        root["email"]=email;
        beast::ostream(connection->_response.body())<<root.dump();

    });

    RegPost("/user_register",[this](std::shared_ptr<HttpConnection> connection){
        HandleUserRegister(connection);
    });


}

LogicSystem::~LogicSystem() {}

void LogicSystem::RegGet(std::string url, HttpHandler handler)  
{ _get_handlers.insert({url, handler}); }
void LogicSystem::RegPost(std::string url, HttpHandler handler) 
{ _post_handlers.insert({url, handler}); }

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con){
    auto it=_get_handlers.find(path);
    if(it==_get_handlers.end()) return false;
    it->second(con);    //找到则执行handler
    return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
    auto it = _post_handlers.find(path);
    if (it == _post_handlers.end()) return false;
    it->second(con);
    return true;
}

void LogicSystem::HandleUserRegister(std::shared_ptr<HttpConnection> conn){

    //1.获取请求体，json
    std::string body_str =
        beast::buffers_to_string(conn->_request.body().data());
        LOG_DEBUG << "receive body is " << body_str;
        conn->_response.set(http::field::content_type, "text/json");

    //2.json解析body
    json root;   //response的body
    json src = json::parse(body_str, nullptr, false);
    if(src.is_discarded()){
        LOG_DEBUG<<"failed to parse JSON data!";
        root["error"]=ErrorCodes::Error_Json;
        beast::ostream(conn->_response.body()) << root.dump();
        return;
    }

    //3.字段齐全判断
    if (!src.contains("email") || !src.contains("user") ||
        !src.contains("passwd") || !src.contains("confirm") ||
        !src.contains("verifycode")) {
        LOG_DEBUG << "register fields missing";
        root["error"] = ErrorCodes::Error_Json;
        beast::ostream(conn->_response.body()) << root.dump();
        return;
    }

    std::string email      = src["email"].get<std::string>();
    std::string user       = src["user"].get<std::string>();
    std::string passwd     = src["passwd"].get<std::string>();
    std::string confirm    = src["confirm"].get<std::string>();
    std::string verifycode = src["verifycode"].get<std::string>();

    //4.密码与确认密码匹配判断
    if (passwd != confirm) {
        LOG_DEBUG << "passwd != confirm";
        root["error"] = ErrorCodes::PasswdErr;
        beast::ostream(conn->_response.body()) << root.dump();
        return;
    }

    //5.验证码匹配
    std::string code_in_redis;
    std::string redis_key="code_"+email;
    bool get_ok=RedisMgr::GetInstance()->Get(redis_key,code_in_redis);
    if(!get_ok){
        LOG_INFO << "verify code expired/missing: " << email;
        root["error"] = ErrorCodes::VerifyExpired;
        beast::ostream(conn->_response.body()) << root.dump();
        return;
    }

    if (code_in_redis != verifycode) {   // 取到了但不相等
        LOG_INFO << "verify code mismatch: " << email;
        root["error"] = ErrorCodes::VerifyCodeErr;
        beast::ostream(conn->_response.body()) << root.dump();
        return;
    }

    //6.查重+mysql建用户
    int uid = 0;   // 桩值
    int reg_res = MysqlMgr::GetInstance()->RegUser(user, email, passwd, uid);
    if (reg_res != ErrorCodes::Success) {
        root["error"] = reg_res;          // UserExist / EmailExist / DB错误
        beast::ostream(conn->_response.body()) << root.dump();
        return;
    }

    //7.组装回包
    root["error"] = ErrorCodes::Success;
    root["email"] = email;
    root["user"]  = user;
    root["uid"]   = uid;
    beast::ostream(conn->_response.body()) << root.dump();
    LOG_INFO  << "user register success. user=" << user << " uid=" << uid;
    return;

}