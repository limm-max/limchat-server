// LogicSystem.cpp
#include "LogicSystem.h"
#include "CSession.h"
#include "const.h"
#include"Logger.h"
#include"RedisMgr.h"
#include"UserMgr.h"
#include"ConfigMgr.h"
#include"MysqlMgr.h"
LogicSystem::LogicSystem():_b_stop(false){
    RegisterCallBacks();
    _worker_thread=std::thread(&LogicSystem::DealMsg,this);
}

LogicSystem::~LogicSystem() {
    _b_stop = true;
    _consume.notify_one();
    _worker_thread.join();
}

//其他io_context线程调用，将消息放入消息队列中
void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg){
    std::unique_lock<std::mutex> lock(_mutex);
    _msg_que.push(msg);
    if(_msg_que.size()==1){
        lock.unlock();
        _consume.notify_one();
    }
}

//优雅退出，使用for循环
void LogicSystem::DealMsg(){
    for(;;){
        std::unique_lock<std::mutex> lock(_mutex);

        //如果队列为空，则cond挂起等唤醒
        while(_msg_que.empty()&& !_b_stop)  _consume.wait(lock);

        // //优雅退出，将剩余msg处理完再停止
        // if(_b_stop){
        //     while(!_msg_que.empty()){
        //         auto msg=_msg_que.front();
        //         auto it=_fun_callbacks.find(msg->_recvnode->_msg_id);
        //         if(it!=_fun_callbacks.end())
        //             it->second(msg->_session,msg->_recvnode->_msg_id,
        //                         std::string(msg->_recvnode->_data, msg->_recvnode->_total_len));
        //         _msg_que.pop();
        //     }
        //     break;
        // }

        // auto msg = _msg_que.front();
        // _msg_que.pop();
        // lock.unlock();                // 处理 handler 时不持锁，别挡住别人入队
        // auto it = _fun_callbacks.find(msg->_recvnode->_msg_id);
        // if (it == _fun_callbacks.end()) continue;   // 没注册的 msg_id 直接丢弃
        // it->second(msg->_session, msg->_recvnode->_msg_id,
        //            std::string(msg->_recvnode->_data, msg->_recvnode->_total_len));

        std::queue<std::shared_ptr<LogicNode>> process_que;
        _msg_que.swap(process_que);
        bool stopping=_b_stop;
        lock.unlock();

        while(!process_que.empty()){
            auto msg = process_que.front();
            process_que.pop();
            auto it = _fun_callbacks.find(msg->_recvnode->_msg_id);
            if (it != _fun_callbacks.end())
                it->second(msg->_session, msg->_recvnode->_msg_id,
                           std::string(msg->_recvnode->_data, msg->_recvnode->_total_len));
        }

        if(stopping)    break;
        
    }
}

void LogicSystem::RegisterCallBacks() {
    _fun_callbacks[MSG_HELLO_WORD] = std::bind(&LogicSystem::HelloWordCallBack, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    _fun_callbacks[MSG_CHAT_LOGIN]= std::bind(&LogicSystem::LoginCallBack, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    _fun_callbacks[MSG_SEARCH_USER]= std::bind(&LogicSystem::SearchUserCallBack, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);  
        
    _fun_callbacks[MSG_ADD_FRIEND_APPLY]= std::bind(&LogicSystem::AddFriendApplyHandler, this,
    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3); 

    _fun_callbacks[MSG_GET_APPLY_LIST]= std::bind(&LogicSystem::GetApplyListHandler, this,
    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    _fun_callbacks[MSG_AUTH_FRIEND_APPLY]= std::bind(&LogicSystem::AuthFriendApplyHandler, this,
    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    _fun_callbacks[MSG_GET_FRIEND_LIST]= std::bind(&LogicSystem::GetFriendListHandler, this,
    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    
}

// echo 占位：把收到的内容原样打包发回，验证整条管路
void LogicSystem::HelloWordCallBack(std::shared_ptr<CSession> session, short msg_id,
                                    const std::string& msg_data) {
    std::cout << "recv msg_id=" << msg_id << " data=" << msg_data << std::endl;
    session->Send(msg_data.c_str(), msg_data.length(), msg_id);   // 原样弹回
}


void LogicSystem::LoginCallBack(std::shared_ptr<CSession> session, short msg_id,
                                    const std::string& msg_data){
    //1.解析body的json
    auto root=json::parse(msg_data,nullptr,false);
    if(root.is_discarded() || !root.contains("uid") || !root.contains("token")){
        LOG_INFO<<"login bad json";
        json rsp;
        rsp["error"]=ErrorCodes::Error_Json;
        if(root.contains("uid"))
            rsp["uid"]=root["uid"];
        if(root.contains("token"))
            rsp["uid"]=root["token"];
        std::string s=rsp.dump();
        session->Send(s.c_str(),static_cast<short>(s.length()),MSG_CHAT_LOGIN_RSP);
        return;
    }

    int         uid   = root["uid"].get<int>();
    std::string token = root["token"].get<std::string>();

    LOG_INFO << "MSG_CHAT_LOGIN uid=" << uid
             << ", token_len=" << token.length();

    //2.使用redis验证uid和token是否匹配
    std::string key=USERTOKENPREFIX+std::to_string(uid);
    std::string token_in_redis;
    bool ok=RedisMgr::GetInstance()->Get(key,token_in_redis);
    if(!ok || token_in_redis!=token){
        LOG_INFO << "token verify failed, uid=" << uid
                 << ", reason=" << (!ok ? "no_key_or_redis_fail" : "mismatch");
        nlohmann::json rsp;
        rsp["error"] = ErrorCodes::TokenInvalid;
        std::string s = rsp.dump();
        session->Send(s.c_str(), static_cast<short>(s.length()), MSG_CHAT_LOGIN_RSP);
        return;
    }
    
    
    //3.比对成功，放行+绑定session
    session->SetUid(uid);

    UserMgr::GetInstance()->SetUserSession(uid, session);   // 进程内 uid→session
    auto self_name = (*ConfigMgr::GetInstance())["SelfServer"]["name"];
    RedisMgr::GetInstance()->Incr(LOGINCOUNTPREFIX + self_name);          // 本服连接数 +1
    RedisMgr::GetInstance()->Set(USERSERVERPREFIX + std::to_string(uid),  // uid→本服
                                 self_name);

    LOG_INFO << "login success, uid=" << uid<<", server=" << self_name;;
    
    nlohmann::json rsp;
    rsp["error"] = ErrorCodes::Success;
    rsp["uid"]   = uid;
    std::string s = rsp.dump();
    session->Send(s.c_str(), static_cast<short>(s.length()), MSG_CHAT_LOGIN_RSP);
}



void LogicSystem::SearchUserCallBack(std::shared_ptr<CSession> session, short msg_id, const std::string& body){
    //1.鉴权守卫
    if(session->GetUid()==0){
        LOG_INFO<<"unauthenticated search request";
        return;
    }

    auto root=json::parse(body,nullptr,false);
    if(root.is_discarded() || !root.contains("uid")){
        json rsp;
        rsp["error"]=ErrorCodes::PasswdInvalid;
        std::string s=rsp.dump();
        session->Send(s.c_str(),static_cast<short>(s.length()),MSG_SEARCH_USER_RSP);
        return;
    }

    int uid=root["uid"].get<int>();
    UserInfo userInfo;
    int ret=MysqlMgr::GetInstance()->SearchUser(uid,userInfo);

    json rsp; 
    rsp["error"]=ErrorCodes::Success;
    rsp["error"] = ret;                  // Success / UserNotExist / MysqlFailed 原样回
    if (ret == ErrorCodes::Success) {
        rsp["uid"]  = userInfo.uid;
        rsp["name"] = userInfo.name;
        rsp["nick"] = userInfo.nick;
        rsp["icon"] = userInfo.icon;
        rsp["sex"]  = userInfo.sex;
    }
    std::string s = rsp.dump();
    session->Send(s.c_str(), static_cast<short>(s.length()), MSG_SEARCH_USER_RSP);

    LOG_INFO << "search user, requester=" << session->GetUid()
             << ", target=" << uid << ", ret=" << ret;
}


void LogicSystem::AddFriendApplyHandler(std::shared_ptr<CSession> session,
                                        short msg_id, const std::string& msg_data) {
    // from_uid 不信客户端传的，用 session 绑定的 uid（防伪造别人发申请）
    int from_uid = session->GetUid();
    if (from_uid == 0) {
        LOG_INFO << "unauthenticated add-friend-apply, session=" << session->GetSessionid();
        return;
    }

    // —— 解析 ——
    auto root = nlohmann::json::parse(msg_data, nullptr, false);
    if (root.is_discarded() || !root.contains("to_uid")) {
        nlohmann::json rsp;
        rsp["error"] = ErrorCodes::ParamInvalid;
        std::string s = rsp.dump();
        session->Send(s.c_str(), static_cast<short>(s.length()), MSG_ADD_FRIEND_APPLY_RSP);
        return;
    }

    
    int to_uid   = root["to_uid"].get<int>();
    std::string apply_msg = root.contains("apply_msg")
                                ? root["apply_msg"].get<std::string>() : "";

    // —— 写表 ——
    int ret = MysqlMgr::GetInstance()->AddFriendApply(from_uid, to_uid, apply_msg);

    // —— 回 A ——
    json rsp;
    rsp["error"] = ret;
    rsp["to_uid"]=to_uid;
    std::string s = rsp.dump();
    session->Send(s.c_str(), static_cast<short>(s.length()), MSG_ADD_FRIEND_APPLY_RSP);

    LOG_INFO << "add friend apply, from=" << from_uid << ", to=" << to_uid << ", ret=" << ret;


    // 第二段：这里之后加"B 在线则推 MSG_NOTIFY_ADD_FRIEND"
    auto to_session = UserMgr::GetInstance()->GetSession(to_uid);
    if (to_session == nullptr) {
        // B 离线：不推。申请已落库，B 下次登录靠 GetApplyList 拉取兜底。
        LOG_INFO << "to_uid=" << to_uid << " offline, notify skipped (will pull on login)";
        return;
    }

    UserInfo applicant;
    int qret=MysqlMgr::GetInstance()->SearchUser(from_uid, applicant);

    json notify;
    notify["from_uid"]  = from_uid;
    notify["apply_msg"] = apply_msg;
    if (qret == 0) {
        notify["name"] = applicant.name;
        notify["nick"] = applicant.nick;
        notify["icon"] = applicant.icon;
        notify["sex"]  = applicant.sex;
    }
    std::string notify_str = notify.dump();
    to_session->Send(notify_str.c_str(),
                     static_cast<short>(notify_str.length()),
                     MSG_NOTIFY_ADD_FRIEND);
}

void LogicSystem::GetApplyListHandler(std::shared_ptr<CSession> session,short msg_id, const std::string& msg_data){
    auto self_uid=session->GetUid();
    if(self_uid==0){
        LOG_INFO<<"GetApplyList unauth, sessionId="<<session->GetSessionid();
        return;
    }

    //msg_data里是self_uid,但不信任
    std::vector<ApplyInfo> list;
    int ret=MysqlMgr::GetInstance()->GetApplyList(self_uid, list);

    json rsp;
    rsp["error"]=ret;
    json arr=json::array();

    for(auto& a:list){
        json item;
        item["from_uid"]  = a.from_uid;
        item["name"]      = a.name;
        item["nick"]      = a.nick;
        item["icon"]      = a.icon;
        item["sex"]       = a.sex;
        item["apply_msg"] = a.apply_msg;
        item["status"]    = a.status;
        arr.push_back(std::move(item));
    }

    rsp["apply_list"]=std::move(arr);

    std::string s=rsp.dump();
    session->Send(s.c_str(), static_cast<short>(s.length()), MSG_GET_APPLY_LIST_RSP);
}

void LogicSystem::AuthFriendApplyHandler(std::shared_ptr<CSession> session,
                                         short msg_id, const std::string& msg_data) {

    auto self_uid = session->GetUid();                 // 认证方 B（= friend_apply.to_uid）
    if (self_uid == 0) { 
        LOG_INFO << "AuthFriendApply unauth, sessionId="<<session->GetSessionid(); 
        return; }

    auto root = json::parse(msg_data, nullptr, false);
    if (root.is_discarded() || !root.contains("from_uid")) {
        LOG_INFO << "AuthFriendApply bad json"; 
        return;
    }

    int  from_uid = root["from_uid"].get<int>();       // 申请方 A
    bool agree    = root.value("agree", false);

    int ret = MysqlMgr::GetInstance()->AuthFriendApply(from_uid, self_uid, agree);

    // 回认证方 B
    json rsp;
    rsp["error"]    = ret;
    rsp["from_uid"] = from_uid;
    rsp["agree"]    = agree;
    std::string rsp_str = rsp.dump();
    session->Send(rsp_str.c_str(), static_cast<short>(rsp_str.length()),
                  MSG_AUTH_FRIEND_APPLY_RSP);

    if (ret != 0 || !agree) return;   // 失败或拒绝：不反向通知 A

    // 同意：反向通知申请方 A。在线则把 B 的资料推过去，A 端即可把 B 加进好友列表
    auto from_session = UserMgr::GetInstance()->GetSession(from_uid);
    if (from_session == nullptr) {
        LOG_INFO << "applicant " << from_uid << " offline, auth-notify skipped";
        return;                       // A 下次登录靠 GetFriendList 兜底
    }
    UserInfo self_info;

    //将同意方B的资料发给A
    int qret = MysqlMgr::GetInstance()->SearchUser(self_uid, self_info);  // 复用 C.1 DAO
    json notify;
    notify["from_uid"] = self_uid;    // 站在 A 视角，新好友是 B
    if (qret == 0) {
        notify["name"] = self_info.name;
        notify["nick"] = self_info.nick;
        notify["icon"] = self_info.icon;
        notify["sex"]  = self_info.sex;
    }
    std::string notify_str = notify.dump();
    from_session->Send(notify_str.c_str(), static_cast<short>(notify_str.length()),
                       MSG_NOTIFY_AUTH_FRIEND);
}


void LogicSystem::GetFriendListHandler(std::shared_ptr<CSession> session,
                                       short msg_id, const std::string& msg_data) {
    auto self_uid = session->GetUid();
    if (self_uid == 0) { LOG_INFO << "GetFriendList unauth, drop"; return; }

    std::vector<FriendInfo> list;
    int ret = MysqlMgr::GetInstance()->GetFriendList(self_uid, list);

    json rsp;
    rsp["error"] = ret;        // 错误码上线统一转 int
    json arr = json::array();
    for (auto& f : list) {
        json item;
        item["uid"]  = f.uid;
        item["name"] = f.name;
        item["nick"] = f.nick;
        item["icon"] = f.icon;
        item["sex"]  = f.sex;
        item["back"] = f.back;
        arr.push_back(std::move(item));
    }
    rsp["friend_list"] = std::move(arr);

    std::string s = rsp.dump();
    session->Send(s.c_str(), static_cast<short>(s.length()), MSG_GET_FRIEND_LIST_RSP);
}