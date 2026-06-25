#pragma once
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <memory>
#include <cstdint>

class MysqlConnection{
public:
    MysqlConnection(sql::Connection* conn,int64_t last_time)
        :_conn(conn),_last_oper_time(last_time){}

    std::unique_ptr<sql::Connection> _conn;
    int64_t _last_oper_time;
};