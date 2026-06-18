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