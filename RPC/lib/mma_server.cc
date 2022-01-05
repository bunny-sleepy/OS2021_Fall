// #include "mma_client.h"
#include "mma_server.h"
#include <chrono>
#include <cstdlib>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>

#ifdef BAZEL_BUILD
#include "proto/mma.grpc.pb.h"
#else
#include "mma.grpc.pb.h"
#endif
namespace proj4 {
void MmaServer::SetUp(int phy_num_page){
  mma = new proj4::MemoryManager(phy_num_page);
  now_vir_num = -1;
}
void MmaServer::SetUp(int phy_num_page, int max_vir_page_num){
  mma = new proj4::MemoryManager(phy_num_page);
  max_vir_num = max_vir_page_num;
  now_vir_num = 0;
}
Status MmaServer::ReadPage(ServerContext* context, const ReadRequest* request, ReadReply* reply){
  int array_id = request->array_id(); 
  int virtual_pid = request->virtual_pid(); 
  int offset = request->offset();
  int result = mma->ReadPage(array_id, virtual_pid, offset);
  reply->set_result(result);
  return Status::OK;
}
Status MmaServer::WritePage(ServerContext* context, const WriteRequest* request, WriteReply* reply){
  int array_id = request->array_id(); 
  int virtual_pd = request->virtual_pid(); 
  int offset = request->offset();
  int value = request->value();
  mma->WritePage(array_id, virtual_pd, offset, value);
  reply->set_result(1);
  return Status::OK;
}
Status MmaServer::Allocate(ServerContext* context, const AllocateRequest* request, AllocateReply* reply){
  int size = request->size();
  int idx = mma->Allocate(size);
  if (now_vir_num == max_vir_num){
    return Status::CANCELLED;
  }
  reply->set_idx(idx);
  if (now_vir_num != -1) {
    std::unique_lock<std::mutex> lock(mut);
    int page_n = (size + PageSize - 1)/PageSize; 
    now_vir_num += page_n;
  }
  return Status::OK;
}
Status MmaServer::Free(ServerContext* context, const FreeRequest* request, FreeReply* reply){
  int idx = request->idx();
  int released_page = mma->Release(idx);
  now_vir_num -= released_page;
  return Status::OK;
}

std::unique_ptr<Server> server;

void RunServerUL(size_t phy_page_num){
  std::string server_address("0.0.0.0:50051");
  MmaServer service;
  service.SetUp(phy_page_num);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  server = std::unique_ptr<Server>(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

void RunServerL(size_t phy_page_num, size_t max_vir_page_num){
  std::string server_address("0.0.0.0:50051");
  MmaServer service;
  service.SetUp(phy_page_num, max_vir_page_num);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  server = std::unique_ptr<Server>(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

void ShutdownServer(){
  server->Shutdown();
}
}