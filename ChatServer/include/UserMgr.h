//管理uid与session之间的映射
#include "Singleton.h"
#include <unordered_map>
#include <memory>
#include <mutex>
class CSession;

class UserMgr : public Singleton<UserMgr> {
    friend class Singleton<UserMgr>;
public:
    ~UserMgr() = default;

    std::shared_ptr<CSession> GetSession(int uid);
    void SetUserSession(int uid, std::shared_ptr<CSession> session);
    
    // 只有当前映射就是这条 session 时才删——防多端登录时"旧连接断开误删了新连接"
    void RemoveUserSession(int uid, std::shared_ptr<CSession> session);

private:
    UserMgr() = default;
    std::mutex _mtx;
    std::unordered_map<int, std::shared_ptr<CSession>> _uid_to_session;
};