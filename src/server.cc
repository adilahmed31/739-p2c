#include "helloworld.grpc.pb.h"
#include "helper.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

constexpr bool CERR_SERVER_CALLS = true;

template <class... T>
void cerr_errors(const T&... args) {
    if constexpr (CERR_SERVER_CALLS)
        (std::cerr << ... << args) << '\n';
}

class BasicRPCServiceImpl final : public BasicRPC::Service
{
    Status creat(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_errors(__PRETTY_FUNCTION__);
        reply->set_value(::creat(req->path().c_str(), req->flag()));
        return Status::OK;
    }

    Status mkdir(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_errors(__PRETTY_FUNCTION__);
        reply->set_value(::mkdir(req->path().c_str(), req->flag()));
        return Status::OK;
    }

    Status rmdir(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_errors(__PRETTY_FUNCTION__);
        reply->set_value(::rmdir(req->path().c_str()));
        return Status::OK;
    }

    Status rm(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_errors(__PRETTY_FUNCTION__);
        reply->set_value(::remove(req->path().c_str()));
        return Status::OK;
    }

    Status stat(ServerContext* context, const PathNFlag* req
                           , Stat* reply) override
    {
        cerr_errors(__PRETTY_FUNCTION__);
        const auto stat = get_stat(req->path().c_str());
        set_time(reply->mutable_atim(), stat.st_atim);
        set_time(reply->mutable_mtim(), stat.st_mtim);
        set_time(reply->mutable_ctim(), stat.st_ctim);
        return Status::OK;
    }

private:
    static void set_time(helloworld::Time* ret, const struct timespec& ts) {
        ret->set_sec(ts.tv_sec);
        ret->set_nsec(ts.tv_nsec);
    }
    static struct stat get_stat(const char* path) {
        struct stat st;
        ::stat(path, &st);
        return st;
    }

//    Status SayServerStreamingString(ServerContext* context, const MessageInt* request
//                                 , ServerWriter< ::helloworld::MessageString>* writer) override
//    {
//        MessageString res;
//        int bytes = request->value();
//
//        int packet_size = std::min(65536, bytes);
//        int timesFull = (bytes / packet_size);
//        bool timesLeft = (bytes % packet_size) > 0;
//
//        std::string packetFull = std::string(packet_size, 'X');
//        std::string packetLeft = std::string(bytes % packet_size, 'X');
//
//        auto start = std::chrono::high_resolution_clock::now();
//        for (int j = 0; j < timesFull; ++j) {
//            res.set_value(packetFull);
//            writer->Write(res);
//        }
//        if (timesLeft)
//        {
//            res.set_value(packetLeft);
//            writer->Write(res);
//        }
//       // std::cout << "SayServerStreamingInt::Server " << request->value() << std::endl;
//        return Status::OK;
//    }
//    Status SayClientStreamingString(ServerContext* context, ServerReader< MessageString>* reader,
//                                 MessageInt* response) override
//    {
//        // Data we are sending to the server.
//        MessageString req;
//        long long int tot = 0;
//        while(reader->Read(&req))
//        {
//            tot += req.value().size();
//        }
//        response->set_value(tot);
//        //std::cout << "SayClientStreamingInt::Server " << tot << std::endl;
//        return Status::OK;
//    }

};

void sigintHandler(int sig_num)
{
    std::cerr << "Clean Shutdown\n";
    //    if (srv_ptr) {
    //        delete srv_ptr;
    //    }
    fflush(stdout);
    std::exit(0);
}

void run_server()
{
    std::string server_address("localhost:50051");
    BasicRPCServiceImpl service;
    //  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.SetMaxSendMessageSize(INT_MAX);
    builder.SetMaxReceiveMessageSize(INT_MAX);
    builder.SetMaxMessageSize(INT_MAX);

    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}
#include <sys/stat.h>
int main()
{

    // "ctrl-C handler"
    signal(SIGINT, sigintHandler);

    struct stat st;

    run_server();
}
