#pragma once

#define FUSE_USE_VERSION 35
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
#include "cache_store.h"
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientWriter;

using grpc::Status;
struct fd_data {
    bool dirty;
    bool has_read;
    int16_t buf_ptr;
    
    size_t bytes;
    struct BufferMem {
        char buf[4096];
        int fd;
    };
    constexpr static int MAX_BUF = 400;
    static BufferMem buf[MAX_BUF];
    static int buf_idx;
    static BufferMem* get_buf() {
        buf_idx = (buf_idx + 1) % MAX_BUF;
        buf[buf_idx].fd = -1;
        return buf + buf_idx;
    };
    BufferMem* buffer;
    
    fd_data() {
        ::bzero(this, sizeof(*this));
    }
};

class BasicRPCClient
{
public:
    static fd_data fds[1 << 16];
    struct OpenRet {
        int ret;
        size_t size;
    };
    BasicRPCClient(std::shared_ptr<Channel> channel)
            : stub_(BasicRPC::NewStub(channel)) {}
    
    int c_create(const std::string& path, int flag);
    int c_mkdir(const std::string& path);
    int c_rm(const std::string& path, int flag);
    int c_rmdir(const std::string& path);
    helloworld::ReadDirResp c_readdir(const std::string& path);
    Stat c_stat(const std::string& path);
    int c_open(const std::string& path, int flag);
    int c_flush(const char* path, int fd);
    int c_unlink(const char* path);
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
    static PathNFlag get(const std::string& path, int flag = 0777);
    std::unique_ptr<BasicRPC::Stub> stub_;
};

