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
#include <memory>
#include<string>
#include <vector>

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
    if (con == nullptr) {
        LOG_DEBUG<<"获取连接失败";
        return ErrorCodes::MysqlFailed;   
    }

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
        return ErrorCodes::MysqlFailed;                  
    } catch(std::exception& e){
        LOG_ERROR<<"Connection Reply Failed:"<<e.what();
        return ErrorCodes::MysqlFailed;
    }
}

//根据name从库中查询——取库中密码——与传入的密码对比——填入UserINfo
int MysqlDao::CheckPwd(const std::string& identifier, const std::string& pwd, UserInfo& userInfo) {
    //1.借连接
    auto conn=_pool->getConnection();
    if(conn==nullptr){
        LOG_ERROR<<"mysql pool is empty.";
        return ErrorCodes::MysqlFailed;
    }

    Defer defer([this,&conn](){
        _pool->returnConnection(std::move(conn));
    });


    try{
        //2.预编译语句
        bool is_email=(identifier.find('@')!=std::string::npos);
        std::string sql = is_email
            ? "SELECT * FROM users WHERE email = ?"
            : "SELECT * FROM users WHERE name  = ?";

        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_conn->prepareStatement(sql));
        
        pstmt->setString(1,identifier);


        //3.执行语句获得结果
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (!res->next()) {
            return ErrorCodes::UserNotExist;          // 查无此人
        }

        std::string origin_pwd=res->getString("pwd");
        if(pwd!=origin_pwd){
            return ErrorCodes::PasswdInvalid;
        }

        //4.填写用户信息
        userInfo.name  = res->getString("name");
        userInfo.email = res->getString("email");
        userInfo.pwd   = origin_pwd;
        userInfo.uid   = res->getInt("uid");
        return ErrorCodes::Success;
    }
    catch(sql::SQLException& e){
        LOG_ERROR<<"sqlException:"<<e.what()
                 <<"(code:"<<e.getErrorCode()
                 <<",state:"<<e.getSQLState()<<")";
        return ErrorCodes::MysqlFailed;
    }
}


int MysqlDao::SearchUser(int uid,UserInfo& userInfo){
    //1.借连接
    auto conn=_pool->getConnection();
    if(conn==nullptr){
        LOG_ERROR<<"mysql is empty";
        return ErrorCodes::MysqlFailed;
    }

    Defer defer([this,&conn](){
        _pool->returnConnection(std::move(conn));});
    
    try{
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->_conn->prepareStatement("SELECT uid,name,email,nick,icon,sex FROM users WHERE uid=?")
        );

        pstmt->setInt(1,uid);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if(!res->next()){
            LOG_INFO<<"User uid "<<uid<<"is not exist.";
            return ErrorCodes::UserNotExist;
        }

        userInfo.uid   = res->getInt("uid");
        userInfo.name  = res->getString("name");
        userInfo.email = res->getString("email");
        userInfo.nick  = res->getString("nick");
        userInfo.icon  = res->getString("icon");
        userInfo.sex   = res->getInt("sex");

        return ErrorCodes::Success;
    }catch(sql::SQLException& e){
        LOG_ERROR<<"SearchUser SQL error:"<<e.what()
                 <<",uid="<<uid;
        return ErrorCodes::MysqlFailed;
    }


}

int MysqlDao::AddFriendApply(int from_uid, int to_uid, const std::string& apply_msg) {
    auto con = _pool->getConnection();
    if (con == nullptr) {
        return ErrorCodes::MysqlFailed;
    }
    Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });

    try {
        std::unique_ptr<sql::PreparedStatement> ps(con->_conn->prepareStatement(
            "INSERT INTO friend_apply (from_uid, to_uid, apply_msg, status) "
                "VALUES (?, ?, ?, 0) "
                "ON DUPLICATE KEY UPDATE apply_msg = VALUES(apply_msg), status = 0"));
        ps->setInt(1, from_uid);
        ps->setInt(2, to_uid);
        ps->setString(3, apply_msg);
        ps->executeUpdate();
        return ErrorCodes::Success;
    }
    catch (sql::SQLException& e) {
        LOG_ERROR << "AddFriendApply SQL error: " << e.what()
                  << ", from=" << from_uid << ", to=" << to_uid;
        return ErrorCodes::MysqlFailed;
    }
}

int MysqlDao::GetApplyList(int to_uid,std::vector<ApplyInfo>& list){
    auto con=_pool->getConnection();
    if(con==nullptr) return ErrorCodes::MysqlFailed;

    Defer defer([this,&con](){
        _pool->returnConnection(std::move(con));
    });

    try{
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_conn->prepareStatement(
                "SELECT a.from_uid,a.apply_msg,a.status,"
                "u.name,u.nick,u.icon.u.sex"
                "FROM friend_apply a"
                "JOIN users u ON a.from_uid=u.uid"
                "WHERE a.to_uid=? AND a.status=0"
                "ORDER BY a.id DESC"
            )
        );
        pstmt->setInt(1,to_uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while(res->next()){
            ApplyInfo info;
            info.from_uid  = res->getInt("from_uid");
            info.apply_msg = res->getString("apply_msg");
            info.status    = res->getInt("status");
            info.name      = res->getString("name");
            info.nick      = res->getString("nick");
            info.icon      = res->getString("icon");
            info.sex       = res->getInt("sex");
            list.push_back(std::move(info));
        }
        return ErrorCodes::Success;
    }catch(sql::SQLException& e){
        LOG_ERROR<<"GetApplyList sql error:"<<e.what();
        return ErrorCodes::MysqlFailed;
    }
}


int MysqlDao::AuthFriendApply(int from_uid,int to_uid,bool agree){
    auto con = _pool->getConnection();
    if (con == nullptr) return ErrorCodes::MysqlFailed;
    Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });

    try{
        //关闭自动提交，即进入事务模式
        con->_conn->setAutoCommit(false);

        //1.更新申请表
        std::unique_ptr<sql::PreparedStatement> up(
            con->_conn->prepareStatement(
                "UPDATE friend_apply SET status=?"
                "WHERE from_uid=? AND to_uid=?"
            )
        );
        up->setInt(1, agree ? 1 : 2);
        up->setInt(2, from_uid);
        up->setInt(3, to_uid);
        up->executeUpdate();

        //2.同意则写friend表,INSERT IGNORE 容忍已是好友的重复
        if(agree){
            std::unique_ptr<sql::PreparedStatement> ins(
                con->_conn->prepareStatement(
                    "INSERT IGNORE INTO friend (self_id, friend_id) "
                    "VALUES (?, ?), (?, ?)"));
            ins->setInt(1, to_uid);    ins->setInt(2, from_uid);   // (B, A)
            ins->setInt(3, from_uid);  ins->setInt(4, to_uid);     // (A, B)
            ins->executeUpdate();
        }

        con->_conn->commit();
        con->_conn->setAutoCommit(true);                   //还原，连接要回池复用
        return ErrorCodes::Success;
    }catch (sql::SQLException& e) {
        LOG_ERROR << "AuthFriendApply sql error: " << e.what();
        //出错要rollback
        try { con->_conn->rollback(); con->_conn->setAutoCommit(true); } catch (...) {}
        return ErrorCodes::MysqlFailed;
    }
}

int MysqlDao::GetFriendList(int self_uid,std::vector<FriendInfo>& list){
    auto con = _pool->getConnection();
    if (con == nullptr) {
        return ErrorCodes::MysqlFailed;      
    }

    Defer defer([this, &con]() { _pool->returnConnection(std::move(con)); });

        try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->_conn->prepareStatement(
                "SELECT u.uid, u.name, u.nick, u.icon, u.sex, f.back "
                "FROM friend f "
                "JOIN users u ON f.friend_uid = u.uid "
                "WHERE f.self_uid = ? "
                "ORDER BY u.uid ASC"));
        pstmt->setInt(1, self_uid);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next()) {
            FriendInfo info;
            info.uid  = res->getInt("uid");
            info.name = res->getString("name");
            info.nick = res->getString("nick");
            info.icon = res->getString("icon");
            info.sex  = res->getInt("sex");
            info.back = res->getString("back");
            list.push_back(std::move(info));
        }
        return ErrorCodes::Success;              // 空列表也是 Success（新用户没好友很正常）
    } catch (sql::SQLException& e) {
        LOG_ERROR << "GetFriendList sql error: " << e.what();
        return ErrorCodes::MysqlFailed;
    }
}