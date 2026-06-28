//业务数据层，负责不同业务的方法。持有mysqlpool
#pragma once
#include"MysqlPool.h"
#include"const.h"
#include<memory>
#include<string>
#include <vector>

class MysqlDao{
public:
    MysqlDao();
    ~MysqlDao();

    //————————操作方法————————————————————————————————
    //注册用户，返回ErrorCodes
    int RegUser(const std::string& name, const std::string& email,
                const std::string& pwd, int& uid); 

    int CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);

    int SearchUser(int uid,UserInfo& userinfo);

    int AddFriendApply(int from_uid, int to_uid, const std::string& apply_msg);

    int GetApplyList(int to_uid,std::vector<ApplyInfo>& list);

    int AuthFriendApply(int from_uid,int to_uid,bool agree);

    int GetFriendList(int self_uid,std::vector<FriendInfo>& list);

private:
    std::unique_ptr<MysqlPool> _pool;
};