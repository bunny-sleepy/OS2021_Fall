#ifndef MMA_SERVER_H
#define MMA_SERVER_H

#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include "memory_manager.h"

#include <grpc++/grpc++.h>
#include <grpc++/ext/proto_server_reflection_plugin.h>
#include <grpc++/health_check_service_interface.h>

#ifdef BAZEL_BUILD
#include "proto/mma.grpc.pb.h"
#else
#include "mma.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using mma::AllocateReply;
using mma::AllocateRequest;
using mma::FreeReply;
using mma::FreeRequest;
using mma::ReadReply;
using mma::ReadRequest;
using mma::RemoteMma;
using mma::WriteReply;
using mma::WriteRequest;


#define PageSize 1024

// Logic and data behind the server's behavior.

namespace proj4 {

class MmaServer final : public RemoteMma::Service {
public:
  void SetUp(int phy_page_num);
  void SetUp(int phy_page_num, int max_vir_page_num);
  Status ReadPage(ServerContext* context, const ReadRequest* request,
                  ReadReply* reply);
  Status WritePage(ServerContext* context, const WriteRequest* request,
                  WriteReply* reply);
  Status Allocate(ServerContext* context, const AllocateRequest* request,
                  AllocateReply* reply);
  Status Free(ServerContext* context, const FreeRequest* request,
                  FreeReply* reply);

private:
  proj4::MemoryManager *mma;
  int max_vir_num;
  int now_vir_num;
  std::mutex mut;
};
// setup a server with UnLimited virtual memory space
void RunServerUL(size_t phy_page_num);

// setup a server with Limited virtual memory space
void RunServerL(size_t phy_page_num, size_t max_vir_page_num);

// shutdown the server setup by RunServerUL or RunServerL
void ShutdownServer();

} //namespace proj4

#endif