#include "client.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int BasicRPCClient::c_open(const std::string& path, int flag) {
    flag = 0777;
    std::cerr << __PRETTY_FUNCTION__ << "\n";
    ClientContext context;
    // TODO: send cached file ts here as well !!!
    PathNFlag req;
    req.set_path(path);
    req.set_flag(flag);
    using pFile =  helloworld::File;
    std::unique_ptr <ClientReader<pFile>> reader(
                stub_->s_open(&context, req));
    const auto cached_tmp_path = get_tmp_cache_path(path);
    const auto cached_path = get_cache_path(path);
    std::ofstream fs(cached_tmp_path, std::ios::binary);

    pFile fstream;
    reader->Read(&fstream);
    log_client(__PRETTY_FUNCTION__, " ", cached_tmp_path, " => ",
            cached_path, " ", fstream.status());

    if (fstream.status() == (int)FileStatus::OK) {
        std::cerr << "Reading file\n";
        while (reader->Read(&fstream))
            fs << fstream.byte();
        fs.close();
        Status status = reader->Finish();
        if (!status.ok()) {
            // TODO: handle gRPC error
        }
        auto ret = ::rename(cached_tmp_path.c_str(), cached_path.c_str());
        std::cerr << "after rename -> "  << ret << " " << errno << "\n";
    } else {
        // TODO:
    }
    const auto ret = ::open(cached_path.c_str(), O_RDWR);
    char buf[100];
    readlink((std::string("/proc/self/fd/") + std::to_string(ret)).c_str() , buf, sizeof(buf));
    log_client("returning fd = ", ret, " -> ", buf);
    return ret;
}

int BasicRPCClient::c_create(const std::string& path, int flag) {
    auto reply = 
    call_grpc([&](ClientContext* c, const PathNFlag& f,
            Int* r)
            {
               return stub_->s_creat(c, f, r);
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
               return stub_->s_mkdir(c, f, r);
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
               return stub_->s_rm(c, f, r);
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
               return stub_->s_rmdir(c, f, r);
            }, get(path, flag), Int(), 
            __PRETTY_FUNCTION__);
    if (!reply) {
        // some error in grpc
    }
    return reply->value();
}

helloworld::ReadDirResp BasicRPCClient::c_readdir(
                            const std::string& path) {
    using RespType = helloworld::ReadDirResp;
    auto reply = 
    call_grpc([&](ClientContext* c, const PathNFlag& f,
            RespType* r)
            {
               return stub_->s_readdir(c, f, r);
            }, get(path, 0), RespType(),
            __PRETTY_FUNCTION__);
    if (!reply) {
        // some error in grpc
    }
    return *reply;
}

Stat BasicRPCClient::c_stat(const std::string& path) {
    using RespType = Stat;
    auto reply = 
    call_grpc([&](ClientContext* c, const PathNFlag& f,
            RespType* r)
            {
               return stub_->s_stat(c, f, r);
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

