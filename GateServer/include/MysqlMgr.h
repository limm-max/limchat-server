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
    
    
private:
    MysqlMgr() = default;
    MysqlDao _dao;
};