//全局配置，命名空间别名、ErrorCodes与客户端对齐

#pragma once

#include<boost/beast.hpp>
#include<boost/beast/http.hpp>
#include<boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;   // 之后写 beast::flat_buffer，而非 boost::beast::flat_buffer
namespace http  = beast::http;    // http::request，而非 boost::beast::http::request
namespace net   = boost::asio;    // net::io_context，而非 boost::asio::io_context
using tcp = boost::asio::ip::tcp; // tcp::acceptor / tcp::socket / tcp::endpoint
using json=nlohmann::json;

enum ErrorCodes{
    Success=0,

    Error_Json=1001,
    RPCFailed=1002,
    VerifyExpired=1003, //redis中验证码过期或者没有
    VerifyCodeErr=1004,
    EmailExist=1005,     //user已经存在（注册时）
    UserExist=1006,
    PasswdErr=1007,     //密码passwd与验证密码confirm不匹配
    PasswdInvalid = 1008,   // 用户名或密码不匹配
    TokenInvalid=1009,
    ParamInvalid=1010,


    MysqlFailed=2001,
    UserNotExist=2002,
    RedisFailed=2003,

};


struct UserInfo{
    std::string name;
    std::string pwd;
    int uid;
    std::string email;
    std::string icon;
    std::string nick;
    int sex=0;

};

//报文常量定义
#define HEAD_ID_LEN     2
#define HEAD_DATA_LEN   2
#define HEAD_TOTAL_LEN  4
#define MAX_LENGTH      2048


enum MSG_IDS{
    MSG_HELLO_WORD      =1001,

    MSG_CHAT_LOGIN      =1002,
    MSG_CHAT_LOGIN_RSP  =1003,

    MSG_SEARCH_USER     =1004,
    MSG_SEARCH_USER_RSP =1005,

    MSG_ADD_FRIEND_APPLY      = 1006,  // 申请好友
    MSG_ADD_FRIEND_APPLY_RSP  = 1007,  // 申请好友回复
    MSG_NOTIFY_ADD_FRIEND     =1008,    //查询自己的待通过申请好友
};


//redis前缀
const std::string USERTOKENPREFIX="utoken_";
const std::string LOGINCOUNTPREFIX = "logincount_";   // 各服连接数
const std::string USERSERVERPREFIX = "userserver_";   // uid→所在服务器：userserver_<uid>