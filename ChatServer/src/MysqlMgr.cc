// MysqlMgr.cpp
#include "MysqlMgr.h"

//返回错误码
int MysqlMgr::RegUser(const std::string& name, const std::string& email,
                      const std::string& pwd, int& uid) {
    return _dao.RegUser(name, email, pwd, uid);   // 薄转发
}

int MysqlMgr::CheckPwd(const std::string& identifier, const std::string& pwd, UserInfo& userInfo) {
    return _dao.CheckPwd(identifier, pwd, userInfo);
}

int MysqlMgr::SearchUser(int uid,UserInfo& userinfo){
    return _dao.SearchUser(uid,userinfo);
}

int MysqlMgr::AddFriendApply(int from_uid, int to_uid, const std::string& apply_msg){
    return _dao.AddFriendApply(from_uid, to_uid, apply_msg);
}


int MysqlMgr::GetApplyList(int to_uid,std::vector<ApplyInfo>& list){
    return _dao.GetApplyList(to_uid,list);
}

int MysqlMgr::AuthFriendApply(int from_uid,int to_uid,bool agree){
    return _dao.AuthFriendApply(from_uid,to_uid,agree);
}

int MysqlMgr::GetFriendList(int self_uid, std::vector<FriendInfo>& list) {
    return _dao.GetFriendList(self_uid,list);
}