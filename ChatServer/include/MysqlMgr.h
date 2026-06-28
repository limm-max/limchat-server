#pragma once
#include "Singleton.h"
#include "MysqlDao.h"
#include <string>
#include "const.h"

class MysqlMgr : public Singleton<MysqlMgr> {
    friend class Singleton<MysqlMgr>;
public:
    ~MysqlMgr() = default;
    int RegUser(const std::string& name, const std::string& email,
                const std::string& pwd, int& uid);
    
    int CheckPwd(const std::string& identifier, const std::string& pwd, UserInfo& userInfo);

    int SearchUser(int uid,UserInfo& userinfo);

    int AddFriendApply(int from_uid, int to_uid, const std::string& apply_msg);

    int GetApplyList(int to_uid,std::vector<ApplyInfo>& list);

    //处理好友申请——修改两表——apply修改状态——friend增加表项——使用事务
    int AuthFriendApply(int from_uid,int to_uid,bool agree);

    int GetFriendList(int self_uid, std::vector<FriendInfo>& list) ;
    
    
private:
    MysqlMgr() = default;
    MysqlDao _dao;
};