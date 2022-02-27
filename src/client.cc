#include "client.h"

int BasicRPCClient::c_create(const std::string& path, int flag) {
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

int BasicRPCClient::c_mkdir(const std::string& path, int flag) {
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

int BasicRPCClient::c_rm(const std::string& path, int flag) {
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

int BasicRPCClient::c_rmdir(const std::string& path, int flag) {
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

Stat BasicRPCClient::c_stat(const std::string& path) {
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
PathNFlag BasicRPCClient::get(const std::string& path, int flag) {

    PathNFlag pf;
    pf.set_path(path);
    pf.set_flag(flag);
    return pf;
}

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
