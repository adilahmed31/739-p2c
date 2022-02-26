#include "helloworld.grpc.pb.h"
#include "helper.h"

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

const std::string GRPC = "grpc_";
const std::string FIRST = GRPC + "first_";

constexpr bool DISABLE_CERR_ERRORS = false;
constexpr bool PRINT_SERVER_OUT = true;

template <class... T>
void cerr_errors(const T&... args) {
    if constexpr (!DISABLE_CERR_ERRORS)
        (std::cerr << ... << args) << '\n';
}

template <class ReplyT>
void print_server_out(const char* fn, const ReplyT& reply) {
    if constexpr (PRINT_SERVER_OUT)
        (std::cerr << fn << " -> " << reply.get_value() << "\n");
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

    auto c_stat(const std::string& path) {
        using RespType = Stat;
        auto reply = 
        call_grpc([&](ClientContext* c, const PathNFlag& f,
                RespType* r)
                {
                   return stub_->stat(c, f, r);
                }, get(path), RespType(), 
                __PRETTY_FUNCTION__);
        if (!reply) {
            // some error in grpc
        }
        return *reply;
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
    static PathNFlag get(const std::string& path, int flag = 0) {
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
    print_proto_stat(greeter.c_stat("/tmp/a.txt"));
    return 0;
}

