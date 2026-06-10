const grpc        = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');
const nodemailer  = require('nodemailer');
const fs          = require('fs');
const redis_module = require("./redis");
const crypto = require("crypto");
const const_module = require("./const");
//读配置
const config = JSON.parse(fs.readFileSync('config.json', 'utf8'));       
const proto  = grpc.loadPackageDefinition(                                
  protoLoader.loadSync('verify.proto', { keepCase: true })                 //   keepCase:保留 error/email/code 原名
).message;

//可复用发信代理
const transporter = nodemailer.createTransport(config.transport);          



//RPC业务函数
//call：客户端请求，callback：回复客户端的函数。参数类型由 proto 定义，自动生成。
async function GetVerifyCode(call, callback) {                             
  const email = call.request.email;        // call.request = GetVarifyReq
  console.log("收到获取验证码请求, email =", email);
  try {
    const key=const_module.code_prefix+email;  // Redis key
    let code = await redis_module.GetRedis(key);
    if (code === null) {
      // crypto.randomInt(0, 1000000) 生成 0~999999 的密码学安全随机数，
      // padStart(6, "0") 补足 6 位（比如 42 → "000042"），保证总是 6 位数字
      code = crypto.randomInt(0, 1000000).toString().padStart(6, "0");

      // 写入 redis，180 秒过期
      const ok = await redis_module.SetRedisExpire(key, code, 180);
      if (!ok) {
        // 写 redis 失败 → 回个 RedisErr 错误码给 GateServer
        callback(null, { email: email, error: const_module.Errors.RedisErr });
        return;
      }
    }

    //redis无数据，或有数据但过期了（过期了 ioredis 也会返回 null）。都要发邮件。
    await transporter.sendMail({
      from: config.from, to: email,
      subject: 'limchat 验证码',
      text: `您的验证码是 ${code}，请在 180 秒内使用。`,
    });
    console.log("验证码已发送, email =", email, "code =", code);
    callback(null, { email: email, error: const_module.Errors.Success });          // callback(err, GetVarifyRsp)
  } catch (e) {
    console.error('[sendMail failed]', e.message);
    callback(null, { email: email, error: const_module.Errors.SendMailErr });
  }
}

function main() {                                                       
  const server = new grpc.Server();
  server.addService(proto.VerifyService.service, { GetVerifyCode });
  //搬内部端口
  server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), (err) => {
    if (err) return console.error(err);
    console.log('VerifyServer running on 0.0.0.0:50051');
  });
}
main();

//后续：邮箱格式校验、限流、结构化日志、优雅退出、crypto 安全码、配置校验、Redis 存码