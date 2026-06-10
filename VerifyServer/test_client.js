const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');
const proto = grpc.loadPackageDefinition(protoLoader.loadSync('verify.proto', { keepCase: true })).message;
const client = new proto.VerifyService('localhost:50051', grpc.credentials.createInsecure());
client.GetVerifyCode({ email: '2389915964@qq.com' }, (err, rsp) => {
  if (err) return console.error('RPC error:', err.message);
  console.log('RPC response:', JSON.stringify(rsp));
});