#ifndef MMA_CLIENT_H
#define MMA_CLIENT_H

#include <memory>
#include <cstdlib>

#include <grpc++/grpc++.h>

#ifdef BAZEL_BUILD
#include "proto/mma.grpc.pb.h"
#else
#include "mma.grpc.pb.h"
#endif
#include "array_list.h"
using grpc::Channel;
using grpc::ClientContext;
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

namespace proj4 { 

class MmaClient {
    public:
  MmaClient(std::shared_ptr<Channel> channel)
      : stub_(RemoteMma::NewStub(channel)) {}
  int ReadPage(int aid, int vid, int offset);
  void WritePage(int aid, int vid, int offset, int value);
  ArrayList *Allocate(int size);
  void Free(ArrayList *);
private:
  std::unique_ptr<RemoteMma::Stub> stub_;
};

} //namespace proj4

#endif