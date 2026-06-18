#pragma once
#include <grpcpp/grpcpp.h>
#include "status.grpc.pb.h"  


using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;

class StatusServiceImpl final : public StatusService::Service {
public:
    StatusServiceImpl();
    Status GetChatServer(ServerContext* context,
                         const GetChatServerReq* request,
                         GetChatServerRsp* reply) override;
};