// MysqlMgr.cpp
#include "MysqlMgr.h"

int MysqlMgr::RegUser(const std::string& name, const std::string& email,
                      const std::string& pwd, int& uid) {
    return _dao.RegUser(name, email, pwd, uid);   // 薄转发
}