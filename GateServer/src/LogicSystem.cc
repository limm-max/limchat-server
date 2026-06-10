#include "LogicSystem.h"
#include "HttpConnection.h" 
#include "VerifyGrpcClient.h" 


LogicSystem::LogicSystem(){
    RegPost("/get_varifycode",[](std::shared_ptr<HttpConnection> connection){

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