const Redis = require("ioredis");
const config = require("./config.json");

const RedisCli = new Redis({
  host: config.redis.host,        // 127.0.0.1
  port: config.redis.port,        // 6379
  password: config.redis.passwd,  // ""（无密码时 ioredis 会忽略它）
});

// 挂一个错误监听。redis 连不上时 ioredis 会触发 "error" 事件，
// 如果不监听，未捕获的错误会让整个 Node 进程崩掉。
RedisCli.on("error", function (err) {
  console.log("RedisCli connect error:", err);
});

//查值
async function GetRedis(key) {
  try {
    // await：等 redis 返回结果。get 不存在的 key 时 ioredis 返回 null
    const result = await RedisCli.get(key);
    if (result === null) {
      console.log(`GetRedis: key [${key}] 不存在`);
      return null;
    }
    console.log(`GetRedis: key [${key}] = ${result}`);
    return result;
  } catch (error) {
    console.log("GetRedis error:", error);
    return null;
  }
}

//存值
async function SetRedisExpire(key, value, ttl) {
  try {
    // 关键：用一条命令同时设值+过期，原子操作。
    // "EX" 表示过期单位是秒，ttl 是秒数。
    // 比「先 set 再 expire 两步」更安全——不会出现 set 成功但 expire 失败、
    // 导致验证码永不过期的缝隙。
    await RedisCli.set(key, value, "EX", ttl);
    return true;
  } catch (error) {
    console.log("SetRedisExpire error:", error);
    return false;
  }
}

//优雅退出
function Quit() {
  RedisCli.quit(function (err) {
    if (err) {
      console.log("RedisCli quit error:", err);
    } else {
      console.log("RedisCli quit successfully");
    }
  });
}

// 导出接口
module.exports = { GetRedis, SetRedisExpire, Quit };