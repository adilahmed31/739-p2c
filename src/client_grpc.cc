#include "client.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int BasicRPCClient::c_release(const std::string& path, int flag){
    flag = 0777;
    std::cerr << __PRETTY_FUNCTION__ << "\n";
    ClientContext context;
    helloworld::File file;
    
    helloworld::Int result;
    
    constexpr int sz = 1 << 16;
    static thread_local char* buffer = new char[sz];
    //const auto cached_tmp_path = get_tmp_cache_path(path);
    const auto cached_path = get_cache_path(path); 
    std::cout << "Client: file transfer " << path << " " << cached_path <<std::endl;
    std::unique_ptr<ClientWriter<helloworld::File>> writer(
                    stub_->s_release(&context, &result) );
    file.set_path(path);  
    std::ifstream fs(cached_path.c_str());
    while (!fs.eof()) {
        fs.read(buffer, sz);
        file.set_byte(buffer);
        writer->Write(file);    
    }
    fs.close();
    //if(!reply){
        //grpc error
    //}
    return 0;
}
    
   
int BasicRPCClient::c_fetch(const std::string& path, int flag){
    std::cerr << __PRETTY_FUNCTION__ << '\n';
    int fd;
    
    const auto cached_path = get_cache_path(path);
    
    char* cache_path = const_cast<char*>(cached_path.c_str());
    auto reply = 
    call_grpc([&](ClientContext* c, const PathNFlag& f,
            File* r)
            {
               return stub_->s_fetch(c, f, r);
            }, get(path, flag), File(), 
            __PRETTY_FUNCTION__);
    if (!reply) {
        // some error in grpc
    }
    if (reply->status() == (int)FileStatus::OK){
        fd = ::creat(cache_path,0);
        lseek(fd, 0, SEEK_SET);
        int size = (int)(reply->size()); 
        std::cerr << reply->byte()<< "  " <<size <<'\n';
        write(fd, &(reply->byte()), size);
        return fd;
    } 
    else{
        fd = ::open(cache_path, O_RDWR, 0);
        return fd;

}
}

    
    
int BasicRPCClient::c_open(const std::string& path, int flag) {
    flag = 0777;
    std::cerr << __PRETTY_FUNCTION__ << "\n";
    ClientContext context;
    // todo: send cached file ts here as well !!!
    helloworld::PathNFlag req;
    req.set_path(path);
    req.set_flag(flag);
    using pfile =  helloworld::File;
    std::unique_ptr <ClientReader<pfile>> reader(
                stub_->s_open(&context, req));
    const auto cached_tmp_path = get_tmp_cache_path(path);
    const auto cached_path = get_cache_path(path);
    std::ofstream fs(cached_tmp_path, std::ios::binary);
    pfile fstream;
    reader->Read(&fstream);
    log_client(__PRETTY_FUNCTION__, " ", cached_tmp_path, " => ",
            cached_path, " ", fstream.status());

    if (fstream.status() == (int)FileStatus::OK) {
        std::cerr << "reading file\n";
        while (reader->Read(&fstream))
            fs << fstream.byte();
        fs.close();
        Status status = reader->Finish();
        if (!status.ok()) {
            // todo: handle grpc error
        }
        auto ret = ::rename(cached_tmp_path.c_str(), cached_path.c_str());
        std::cerr << "after rename -> "  << ret << " " << errno << "\n";
    } else {
        //todo
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
    if (reply->value() != 0)
        std::cerr <<"Directory could not be created!. Error code: " << reply->value() << "\n";
    if (!reply) {
        // some error in grpc
    }
    return reply->value();
}

int BasicRPCClient::c_access(const std::string& path, int flag){
    auto reply = 
    call_grpc([&](ClientContext* c, const PathNFlag& f, Int* r){
        return stub_->s_access(c,f,r);
    }, get(path, flag), Int(), 
    __PRETTY_FUNCTION__);
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

