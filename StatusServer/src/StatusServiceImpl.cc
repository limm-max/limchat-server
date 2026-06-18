#include "StatusServiceImpl.h"
#include "const.h"
#include "Logger.h"
#include "ConfigMgr.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "RedisMgr.h"
//token随机生成器
std::string generate_unique_string() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return boost::uuids::to_string(uuid);
}


StatusServiceImpl::StatusServiceImpl() {}


Status StatusServiceImpl::GetChatServer(ServerContext* context,
                                        const GetChatServerReq* request,
                                        GetChatServerRsp* reply) {

    
    auto uid=request->uid();
    LOG_INFO<< "GetChatServer called, uid = " << uid;

    //1.选择一台ChatServer
    //TODO:当前：直接选择配置的，后续ChatServer创建之后通过读redis的连接数再修改
    auto cfg = ConfigMgr::GetInstance();          // ← 用你实际的取实例写法
    std::string host = (*cfg)["ChatServer1"]["host"];
    std::string port = (*cfg)["ChatServer1"]["port"];

    //2.生成token
    std::string token = generate_unique_string();

    // 3. 写进 Redis：key = utoken_<uid>，供 ChatServer 日后直接读取校验
    std::string token_key = USERTOKENPREFIX + std::to_string(uid);
    bool is_ok=RedisMgr::GetInstance()->Set(token_key, token);
    if(!is_ok){
        reply->set_error(ErrorCodes::RedisFailed);
        LOG_ERROR<<"GetChatServer reply failed.";
        return Status::OK;
    }   

    // 4. 填回包
    reply->set_error(ErrorCodes::Success);
    reply->set_host(host);
    reply->set_port(port);
    reply->set_token(token);
    LOG_INFO<<"GetChatServer response,token="<<token;
    return Status::OK;
}


