// #include "mma_client.h"
#include "mma_client.h"
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
namespace proj4{ 

int MmaClient::ReadPage(int array_id, int virtual_pid, int offset) {
  ReadRequest request;
  request.set_array_id(array_id);
  request.set_virtual_pid(virtual_pid);
  request.set_offset(offset);

  ReadReply reply;
  ClientContext context;
  Status status = stub_->ReadPage(&context, request, &reply);

  if (status.ok()) {
    return reply.result();
  } 
  else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    return -1;
  }
}
void MmaClient::WritePage(int array_id, int virtual_pid, int offset, int value) {
  WriteRequest request;
  request.set_array_id(array_id);
  request.set_virtual_pid(virtual_pid);
  request.set_offset(offset);
  request.set_value(value);

  WriteReply reply;
  ClientContext context;
  Status status = stub_->WritePage(&context, request, &reply);

  if (status.ok()) {
    return;
  } 
  else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    return;
  }
}

ArrayList *MmaClient::Allocate(int size) {
  while(true){
    AllocateRequest request;
    request.set_size(size);

    AllocateReply reply;
    ClientContext context;
    Status status = stub_->Allocate(&context, request, &reply);

    if (status.ok()) {
      return new ArrayList(size, this, reply.idx());
    } 
    else {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}
void MmaClient::Free(ArrayList *arr) {
  FreeRequest request;
  request.set_idx(arr->array_id);

  FreeReply reply;
  ClientContext context;
  Status status = stub_->Free(&context, request, &reply);

  if (status.ok()) {
    return;
  } 
  else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    return;
  }
}}