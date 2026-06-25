







板块二：分布式服务
ChatServer 多实例化 — 同一份代码 + 不同 config(端口/服务器名),起多台
StatusServer 负载均衡 — GetChatServer 读各台 logincount_<server>(B.3 已埋),选连接数最少的返回(当前是写死返回一台)
跨服消息转发(ChatService gRPC) — A 在服务器1、B 在服务器2 时,服务器1 通过 gRPC 把消息转发给服务器2,由它推给 B。这是整个项目"分布式"含金量最高处


板块三:健壮性 / 进阶

心跳检测 — 检测半开连接(对端崩了但 TCP 没断),定时 ping/pong,超时清理
断线重连 — 客户端掉线后指数退避重连 + 重连后重发 TCP 登录
单账号多端登录踢人 — 同一 uid 在新设备登录,踢掉旧 session(配合分布式锁防竞态)
分布式锁 — 多台 ChatServer 场景下,对同一 uid 的上线/踢人操作加 Redis 分布式锁,防并发竞态(如两端几乎同时登录)
文件 / 多媒体传输 — 图片、文件消息(独立的传输通道或分片协议)
性能压测 — 自己压测得出连接数/吞吐数字(简历红线:绝不借 llfc 的数字)
遗留债 — token 加 TTL、密码加盐哈希、gRPC 同步阻塞改异步、VerifyServer 限流、重置密码 handler