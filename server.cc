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

class ServerServiceImpl final : public ServerService::Service {
 public:
  Status helloOfServer(ServerContext* context, const HelloRequest* request,
                       HelloReply* reply) override {
    std::cout << "request from " << request->name();
    std::string prefix("Hello ");
    reply->set_msg(prefix + request->name());
    return Status::OK;
  }
};

class SocketServer {
 private:
  std::unique_ptr<ClientService::Stub> client_stub_;
  // std::unique_ptr<ManagerService::Stub> manager_stub_;

 public:
  SocketServer(std::shared_ptr<Channel> channel)
      : client_stub_(ClientService::NewStub(channel)) {}
  // void Register() { manager_stub_->Register(); }
  std::string SayHello() {
    HelloRequest req;
    HelloReply reply;
    ClientContext ctx;
    req.set_name("server_a");
    std::cout << "server side is about to invoke rpc\n";
    // The actual RPC.
    Status status = client_stub_->helloOfClient(&ctx, req, &reply);

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
  std::string server_address("0.0.0.0:50052");
  ServerServiceImpl service;

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
  std::cout << "socketServer listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

void InvokePeer() {
  std::string target_str = "127.0.0.1:50051";
  SocketServer socket_server(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  sleep(5);
  // socket server invoke rpc provided by client.
  // std::cout << "*****\n";
  std::string msg = socket_server.SayHello();
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