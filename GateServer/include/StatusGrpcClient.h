#pragma once
#include "const.h"
#include "Singleton.h"
#include <grpcpp/grpcpp.h>
#include "status.grpc.pb.h"
#include "ConfigMgr.h"
using grpc::Channel; using grpc::ClientContext; using grpc::Status;
using message::GetChatServerReq; using message::GetChatServerRsp; using message::StatusService;

class StatusGrpcClient : public Singleton<StatusGrpcClient>{
    friend class Singleton<StatusGrpcClient>;
public:
    GetChatServerRsp GetChatServer(int uid){
        ClientContext context;
        GetChatServerReq request;
        request.set_uid(uid);
        GetChatServerRsp reply;
        LOG_DEBUG<<"Get uid="<<uid;

        Status status=stub_->GetChatServer(&context,request,&reply);
        if (!status.ok()) {
            LOG_ERROR<<"StatusServer Grpc Failed.";
            reply.set_error(ErrorCodes::RPCFailed);
            
        }
        return reply;

    }

private:
    StatusGrpcClient() {

        //读config
        auto cfg = ConfigMgr::GetInstance();
        std::string host=(*cfg)["StatusServer"]["host"];
        std::string port=(*cfg)["StatusServer"]["port"];
        std::string addr=host + ":" + port;
        LOG_DEBUG<<"Reading StatusServer Add finished,addr="<<addr;

        grpc::ChannelArguments args;
        args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0); 
        auto channel = grpc::CreateCustomChannel(addr,   
                                           grpc::InsecureChannelCredentials(),args);

        stub_ = StatusService::NewStub(channel);   // channel→stub，整个程序复用这一个
    }
    std::unique_ptr<StatusService::Stub> stub_;
};


