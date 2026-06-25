# ChatServer三层设计
## 最外层——TCP服务器框架
* CServer为Acceptor
* CSession为每一个连接对象（TcpConnection）
* IOServicePool，线程池，按 CPU 核数开 N 个 io_context、各跑一个线程,新连接轮询分配
* TLV 协议层处理粘包/半包
    * 项目中的TCP收到的body：msg_id 2字节，body_len 2字节，body（JSON）格式
    * 解决分发：使用async_read「精确读 N 字节」
* LogicSystem:根据msg_id进行handler路由分发机制（logicSystem有对应单一线程进行处理）
    * 改进：将LogicSystem中的单缓冲队列优化成“双缓冲队列”。
    * logicSystem中的handler对应后续上层的业务逻辑

## 中间层——TCP登录+鉴权，TCP与业务之间的桥梁
* 发生时期：login界面使用http与GateServer通信获得对应的chatserver uid、host、port和token；使用这个凭证和chatserver验证
* 第一个Handler，也就是所有chat服务的门槛和开端——LoginHandler，即将一个匿名socket变成已认证的某个用户
* session<->uid，使用对应token等进行验证
* 即MSG_CHAT_LOGIN业务：1.解析body的json，收到uid和token。2.读redis存的uid对应的token，比对验证。3.绑定session（tcpconnection）和uid 

## 最里层——真正的聊天项目
* Mysql表：users、friend_apply，friend，chat_message
业务功能：搜索用户、发起好友申请、同意/拒绝申请、好友列表、单聊发消息、聊天记录、跨服转发 / 跨服踢人
* 每个业务都需要有守卫——在真正的业务handle之前，检验对应session的uid是否存在

### 搜索好友
    Dao添加方法：search_user
    LogicSystem添加callback，回复搜索到的用户信息

### 添加好友
    客户端A发送申请——对应sessionA转发logicsystem——logicsystem增加mysql表项——sessionA发出消息——返回给客户端A
                                                                                        ——sessionB接收——sessionB推送给B
    Dao添加方法：增加friend_apply表项
    LogicSystem添加callback
* friend_apply表： id、from_uid、to_uid、apply_msg、status、
```
CREATE TABLE friend_apply (
    id        INT NOT NULL AUTO_INCREMENT,
    from_uid  INT NOT NULL,              -- 谁发起的申请
    to_uid    INT NOT NULL,              -- 申请加谁
    apply_msg VARCHAR(255) DEFAULT '',   -- 验证消息（"我是xxx"）
    status    INT DEFAULT 0,             -- 0待处理 1同意 2拒绝
    PRIMARY KEY (id),
    KEY idx_to_uid (to_uid)              -- B 查"谁申请加我"要按 to_uid 查，建索引
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

