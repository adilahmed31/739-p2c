#include "helloworld.grpc.pb.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <signal.h>
#include <chrono>
#include <ctime>
#include <vector>
#include <unordered_map>
#include <numeric>
#include <fstream>
#include <memory>
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientWriter;

using grpc::Status;
using helloworld::BasicRPC;

using helloworld::Int;
using helloworld::PathNFlag;
using helloworld::Bytes;

const std::string GRPC = "grpc_";
const std::string FIRST = GRPC + "first_";

constexpr bool DISABLE_CERR_ERRORS = false;

template <class... T>
void cerr_errors(const T&... args) {
    if constexpr (!DISABLE_CERR_ERRORS)
        (std::cerr << ... << args) << '\n';
}


class BasicRPCClient
{
    
public:
    BasicRPCClient(std::shared_ptr<Channel> channel)
            : stub_(BasicRPC::NewStub(channel)) {}
    
    int c_create(const std::string& path, int flag) {
        auto reply = 
        call_grpc([&](ClientContext* c, const PathNFlag& f,
                Int* r)
                {
                   return stub_->creat(c, f, r);
                }, get(path, flag), Int(), 
                __PRETTY_FUNCTION__);
        if (!reply) {
            // some error in grpc
        }
        return reply->value();
    }

    int c_mkdir(const std::string& path, int flag) {
        auto reply = 
        call_grpc([&](ClientContext* c, const PathNFlag& f,
                Int* r)
                {
                   return stub_->mkdir(c, f, r);
                }, get(path, flag), Int(), 
                __PRETTY_FUNCTION__);
        if (!reply) {
            // some error in grpc
        }
        return reply->value();
    }

    int c_rm(const std::string& path, int flag) {
        auto reply = 
        call_grpc([&](ClientContext* c, const PathNFlag& f,
                Int* r)
                {
                   return stub_->rm(c, f, r);
                }, get(path, flag), Int(), 
                __PRETTY_FUNCTION__);
        if (!reply) {
            // some error in grpc
        }
        return reply->value();
    }

    int c_rmdir(const std::string& path, int flag) {
        auto reply = 
        call_grpc([&](ClientContext* c, const PathNFlag& f,
                Int* r)
                {
                   return stub_->rmdir(c, f, r);
                }, get(path, flag), Int(), 
                __PRETTY_FUNCTION__);
        if (!reply) {
            // some error in grpc
        }
        return reply->value();
    }



private:
    template <class ArgT, class ReplyT, class F>
    std::optional<ReplyT> call_grpc(F&& f, const ArgT& arg, 
                    ReplyT&& ,
                    const char* fname) {
        ClientContext context;
        ReplyT reply;
        const auto status = f(&context, arg, &reply);
        if (!status.ok()) {
            cerr_errors("RPC call failed: ",
                    status.error_message(), " | ", fname);
            return std::nullopt;
        }
        return reply;
    }
    static PathNFlag get(const std::string& path, int flag) {
        PathNFlag pf;
        pf.set_path(path);
        pf.set_flag(flag);
        return pf;
    }

    std::unique_ptr<BasicRPC::Stub> stub_;
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

int main(int argc, char *argv[])
{
    // "ctrl-C handler"
    signal(SIGINT, sigintHandler);
    const std::string target_str = "localhost:50051";
    grpc::ChannelArguments ch_args;

    ch_args.SetMaxReceiveMessageSize(INT_MAX);
    ch_args.SetMaxSendMessageSize(INT_MAX);

    BasicRPCClient greeter(
            grpc::CreateCustomChannel(target_str,
            grpc::InsecureChannelCredentials() , ch_args ));

    greeter.c_create("/tmp/a.txt", 0);
    return 0;
}

