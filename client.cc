// g++ server.cc ./gen_code/common.grpc.pb.cc ./gen_code/common.pb.cc -o server
// `pkg-config --cflags protobuf grpc` `pkg-config --libs protobuf grpc++ grpc`
// -pthread -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include "./gen_code/common.grpc.pb.h"

using helloworld::ClientService;
using helloworld::HelloReply;
using helloworld::HelloRequest;
using helloworld::ServerService;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class ClientServiceImpl final : public ClientService::Service {
 public:
  // client rpc
  Status helloOfClient(ServerContext* context, const HelloRequest* request,
                       HelloReply* reply) override {
    std::string prefix("Hello ");
    reply->set_msg(prefix + request->name());
    return Status::OK;
  }
};

class Client {
 private:
  std::unique_ptr<ServerService::Stub> server_stub_;
  // std::unique_ptr<ManagerService::Stub> manager_stub_;
 public:
  Client(std::shared_ptr<Channel> channel)
      : server_stub_(ServerService::NewStub(channel)) {}

  // void Register() { manager_stub_->Register(); }
  std::string SayHello() {
    HelloRequest req;
    HelloReply reply;
    ClientContext ctx;
    req.set_name("client_a");
    std::cout << "client side is about to invoke rpc\n";

    // The actual RPC.
    Status status = server_stub_->helloOfServer(&ctx, req, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.msg();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  ClientServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Client listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
  // return server;
}

void InvokePeer() {
  std::string target_str = "0.0.0.0:50052";
  Client cli(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  sleep(5);

  // client invoke rpc provided by socket server.
  // std::cout << "*****\n";
  std::string msg = cli.SayHello();
  std::cout << "msg from peer: " << msg << "\n";
}

int main() {
  std::thread server_th(RunServer);
  std::thread do_th(InvokePeer);

  // TODO: shutdown the server
  server_th.join();
  do_th.join();

  return 0;
}