//业务数据层，负责不同业务的方法。持有mysqlpool
#pragma once
#include"MysqlPool.h"
#include<memory>
#include<string>

class MysqlDao{
public:
    MysqlDao();
    ~MysqlDao();

    //————————操作方法————————————————————————————————
    //注册用户，返回ErrorCodes
    int RegUser(const std::string& name, const std::string& email,
                const std::string& pwd, int& uid); 

private:
    std::unique_ptr<MysqlPool> _pool;
};