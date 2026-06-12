#include "MysqlDao.h"
#include "Defer.h"
#include "ConfigMgr.h"
#include "const.h"            
#include "Logger.h"
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>
#include <ctime>

MysqlDao::MysqlDao() {

    auto cfg = ConfigMgr::GetInstance();
    std::string host   = (*cfg)["Mysql"]["host"];
    std::string port   = (*cfg)["Mysql"]["port"];
    std::string user   = (*cfg)["Mysql"]["user"];
    std::string pwd    = (*cfg)["Mysql"]["pwd"];
    std::string schema = (*cfg)["Mysql"]["schema"];
    _pool = std::make_unique<MysqlPool>(host, port, user, pwd, schema, 5);
}

MysqlDao::~MysqlDao() {
    _pool->close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email,
                      const std::string& pwd, int& uid) {
    auto con = _pool->getConnection();
    if (con == nullptr) return ErrorCodes::DatabaseFailed;   

    Defer defer([this, &con](){ _pool->returnConnection(std::move(con)); });

    try {
        // 1) 邮箱查重 → EmailExist
        {
            std::unique_ptr<sql::PreparedStatement> ps(con->_conn->prepareStatement(
                "SELECT 1 FROM users WHERE email = ? LIMIT 1"));
            ps->setString(1, email);
            std::unique_ptr<sql::ResultSet> rs(ps->executeQuery());
            if (rs->next()) return ErrorCodes::EmailExist;
        }
        // 2) 用户名查重 → UserExist
        {
            std::unique_ptr<sql::PreparedStatement> ps(con->_conn->prepareStatement(
                "SELECT 1 FROM users WHERE name = ? LIMIT 1"));
            ps->setString(1, name);
            std::unique_ptr<sql::ResultSet> rs(ps->executeQuery());
            if (rs->next()) return ErrorCodes::UserExist;
        }
        // 3) 插入(UNIQUE 约束是真正兜底:并发时如果出现check-then-insert，这一步可catch)
        {
            std::unique_ptr<sql::PreparedStatement> ps(con->_conn->prepareStatement(
                "INSERT INTO users (name, email, pwd) VALUES (?, ?, ?)"));
            ps->setString(1, name);
            ps->setString(2, email);
            ps->setString(3, pwd);
            ps->executeUpdate();
        }
        // 4) 取自增 uid(LAST_INSERT_ID 是连接级,同一 con 内取才准)
        {
            std::unique_ptr<sql::Statement> stmt(con->_conn->createStatement());
            std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT LAST_INSERT_ID()"));
            if (rs->next()) uid = rs->getInt(1);
        }
        con->_last_oper_time = static_cast<int64_t>(time(nullptr));
        return ErrorCodes::Success;

    } catch (sql::SQLException& e) {
        if (e.getErrorCode() == 1062) {                 // 1062 = 唯一键冲突(竞态漏网)
            LOG_WARN << "register duplicate key: " << e.what();
            return ErrorCodes::UserExist;               // 兜底归类
        }
        LOG_ERROR << "RegUser SQLException: " << e.what();
        return ErrorCodes::DatabaseFailed;                   // 建议加个专门 DB 错误码
    }
}