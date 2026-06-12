#pragma once
#include "Singleton.h"
#include "MysqlDao.h"
#include <string>

class MysqlMgr : public Singleton<MysqlMgr> {
    friend class Singleton<MysqlMgr>;
public:
    ~MysqlMgr() = default;
    int RegUser(const std::string& name, const std::string& email,
                const std::string& pwd, int& uid);
private:
    MysqlMgr() = default;
    MysqlDao _dao;
};