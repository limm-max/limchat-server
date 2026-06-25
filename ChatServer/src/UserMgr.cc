// UserMgr.cpp
#include "UserMgr.h"
#include "CSession.h"

std::shared_ptr<CSession> UserMgr::GetSession(int uid) {
    std::lock_guard<std::mutex> lk(_mtx);
    auto it = _uid_to_session.find(uid);
    return it == _uid_to_session.end() ? nullptr : it->second;
}

void UserMgr::SetUserSession(int uid, std::shared_ptr<CSession> session) {
    std::lock_guard<std::mutex> lk(_mtx);
    _uid_to_session[uid] = session;   // 多端登录：新连接覆盖旧（TODO:踢旧端是后续进阶，先不做）
}

void UserMgr::RemoveUserSession(int uid, std::shared_ptr<CSession> session) {
    std::lock_guard<std::mutex> lk(_mtx);
    auto it = _uid_to_session.find(uid);
    
    // 关键守卫：映射里存的还是"我这条"才删。否则旧 session 断开会误删新登录的映射
    if (it != _uid_to_session.end() && it->second == session) {
        _uid_to_session.erase(it);
    }
}