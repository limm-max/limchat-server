#pragma once
#include "const.h"
#include "Singleton.h"
#include <grpcpp/grpcpp.h>
#include "verify.grpc.pb.h"
#include "ConfigMgr.h"
using grpc::Channel; using grpc::ClientContext; using grpc::Status;
using message::GetVerifyReq; using message::GetVerifyRsp; using message::VerifyService;

class VerifyGrpcClient : public Singleton<VerifyGrpcClient>{
    friend class Singleton<VerifyGrpcClient>;
public:
    GetVerifyRsp GetVerifyCode(const std::string& email){
        ClientContext context;
        GetVerifyReq request;
        request.set_email(email);
        GetVerifyRsp reply;

        Status status=stub_->GetVerifyCode(&context,request,&reply);
        if (!status.ok()) reply.set_error(ErrorCodes::RPCFailed);
        return reply;

    }

private:
    VerifyGrpcClient() {

        //读config
        auto cfg = ConfigMgr::GetInstance();
        std::string host=(*cfg)["VerifyServer"]["host"];
        std::string port=(*cfg)["VerifyServer"]["port"];
        std::string addr=host + ":" + port;
        auto channel = grpc::CreateChannel(addr,   
                                           grpc::InsecureChannelCredentials());
        //硬编码
        // auto channel = grpc::CreateChannel("127.0.0.1:50051",   
        //                                    grpc::InsecureChannelCredentials());
        stub_ = VerifyService::NewStub(channel);   // channel→stub，整个程序复用这一个
    }
    std::unique_ptr<VerifyService::Stub> stub_;
};