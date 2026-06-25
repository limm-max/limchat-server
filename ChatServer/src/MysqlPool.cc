#include "MysqlPool.h"
#include "Logger.h"
#include "Defer.h"
#include <cppconn/statement.h>
#include <cppconn/exception.h>
#include <ctime>
#include <chrono>

MysqlPool::MysqlPool(const std::string& host,const std::string& port,
                const std::string& user,const std::string& passwd,
                const std::string& schema,int poolSize)
            :_user(user),_pass(passwd),_schema(schema),_poolSize(poolSize),_b_stop(false)
        {
            _url="tcp://"+host+":"+port;
            sql::mysql::MySQL_Driver* driver=
                        sql::mysql::get_mysql_driver_instance();
 
            for(int i=0;i<_poolSize;++i){
                try{                    
                    sql::Connection* conn=driver->connect(_url,_user,_pass);
                    conn->setSchema(_schema);
                    int64_t now=static_cast<int64_t>(time(nullptr));
                    _pool.push(std::make_unique<MysqlConnection>(conn,now));
                }catch(sql::SQLException& e){
                    LOG_ERROR<<"MysqlPool 初始化创建连接#"<<i<<"失败："<<e.what();
                }
            }
            if (_pool.empty()) {
                LOG_ERROR << "mysql pool empty, check config/grant";  // 一条都没起来,fail loud
            }
            LOG_INFO<<"Mysql pool 初始化结束，池子连接数为:"<<_pool.size();
        }

MysqlPool::~MysqlPool(){
    close();
    std::lock_guard<std::mutex> lock(_mutex);
    while(!_pool.empty())   _pool.pop();

}


std::unique_ptr<MysqlConnection> MysqlPool::getConnection(){
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock,[this](){
        return _b_stop || !_pool.empty();
    });
    if(_b_stop) {
        LOG_DEBUG<<"mysql pool已停止，返回空连接。";
        return nullptr;
    }
    auto conn=std::move(_pool.front());
    _pool.pop();
    return conn;
}


void  MysqlPool::returnConnection(std::unique_ptr<MysqlConnection> conn){
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock,[this](){
        return _b_stop || !_pool.empty();
    });
    if(_b_stop) return;
    _pool.push(std::move(conn));
    _cond.notify_one();
}

void MysqlPool::close(){
    _b_stop=true;
    _cond.notify_all();
    if(_check_thread.joinable())    _check_thread.join();
    LOG_INFO<<"Mysql连接池已关闭.";
}


//TODO:进阶：将锁变成pop出来后还锁，ping之后/重建连接之后push回去
//TODO：加上如果连接断了就重建链接
void MysqlPool::checkConnection(){
    using namespace std::chrono;
    while(!_b_stop){
        //分段sleep，close能及时叫醒
        for(int i=0;i<60 && !_b_stop;++i)
            std::this_thread::sleep_for(seconds(1));
        if(_b_stop) break;

        std::lock_guard<std::mutex> lock(_mutex);
        int size=static_cast<int>(_pool.size());
        int64_t now=static_cast<int64_t>(time(nullptr));
        for(int i=0;i<size;++i){
            auto conn=std::move(_pool.front());
            _pool.pop();
            Defer defer([this,&conn]{
                _pool.push(std::move(conn));
            });
            if(now-conn->_last_oper_time<5) continue;

            try{
                std::unique_ptr<sql::Statement> stmt(conn->_conn->createStatement());
                stmt->execute("SELECT 1");
                conn->_last_oper_time=now;
            }catch(sql::SQLException& e){
                LOG_ERROR << "keepalive ping failed: " << e.what();

                try {
                    auto* driver = sql::mysql::get_mysql_driver_instance();
                    sql::Connection* fresh = driver->connect(_url, _user, _pass);
                    fresh->setSchema(_schema);
                    conn.reset(new MysqlConnection(fresh, now));   // 用新的替换,Defer 还的就是新的
                } catch (sql::SQLException& e2) {
                    LOG_ERROR << "rebuild failed: " << e2.what();
                    conn.reset();   // 重建也失败:置空,别把坏的还回去(Defer 要判 null)
                }
            }

        }
    }
}
