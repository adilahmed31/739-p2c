#include "helloworld.grpc.pb.h"
#include "helper.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <dirent.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

std::string get_server_path(const std::string& path) {
    return "/users/agabhin/.fuse_server/" + path;
}

class BasicRPCServiceImpl final : public BasicRPC::Service
{
    Status s_creat(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_serv_calls(__PRETTY_FUNCTION__);
        const auto path = get_server_path(req->path());
        reply->set_value(::creat(path.c_str(), req->flag()));
        std::ofstream fs(path.c_str()); fs << "test string";
        set_time(reply->mutable_ts(), get_stat(path.c_str()).st_mtim);
        return Status::OK;
    }

    Status s_mkdir(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_serv_calls(__PRETTY_FUNCTION__);
        const auto path = get_server_path(req->path());
        int res = ::mkdir(path.c_str(),req->flag());
        reply->set_value(res);
        if (res == -1){
            //perror(strerror(errno));
            reply->set_value(-errno);
        }
        else{
            reply->set_value(0);
        }
        
        set_time(reply->mutable_ts(), get_stat(path.c_str()).st_mtim);
        return Status::OK;
    }

    Status s_rmdir(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_serv_calls(__PRETTY_FUNCTION__);
        reply->set_value(::rmdir(get_server_path(req->path()).c_str()));
        return Status::OK;
    }

    Status s_rm(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_serv_calls(__PRETTY_FUNCTION__);
        reply->set_value(::remove(get_server_path(req->path()).c_str()));
        return Status::OK;
    }

    Status s_readdir(ServerContext* context, const PathNFlag* req
                           , helloworld::ReadDirResp* reply) override
    {
        DIR *dp;
        struct dirent *de;
        const auto path = get_server_path(req->path());
        cerr_serv_calls("s_readdir on ", path);
        dp = opendir(path.c_str());
        if (dp == nullptr) {
            reply->set_ret_code(-errno);
            return Status::OK;
        }
        while ((de = readdir(dp)) != nullptr) {
            reply->add_names(de->d_name);
        }
        closedir(dp);
        return Status::OK;
    }

    Status s_stat(ServerContext* context, const PathNFlag* req
                           , Stat* reply) override
    {
        const auto path = get_server_path(req->path());
        set_stat(path.c_str(), reply);
        cerr_serv_calls(__PRETTY_FUNCTION__, " -> ", path,
            " sz = ", reply->size());
        return Status::OK;
    }
    
    Status s_fetch(ServerContext* context, const PathNFlag* request, File* reply) override {
        int fd;
        Stat stat;
        const auto path = get_server_path(request->path());
        char* c_path = const_cast<char*>(path.c_str());
        set_stat(path.c_str(), &stat);
        int status = (int)FileStatus :: OK;
        if (stat.mtim() == request->ts()){
               status = ((int)FileStatus::FILE_ALREADY_CACHED);
        }
        if (status == (int)FileStatus::OK){
            int fd = open(path.c_str(), O_RDWR);
            char* buf = (char*)malloc(stat.size());
            ::lseek(fd,0,SEEK_SET);
            ::read(fd, buf, stat.size());
            reply->set_byte(buf);
        }   
        reply->set_size(stat.size());
        reply->set_status(status);
        return Status::OK;
}
    
    Status s_open(ServerContext* context, const PathNFlag* req, 
                    ServerWriter<helloworld::File>* writer) override {
        cerr_serv_calls(__PRETTY_FUNCTION__);
        constexpr int sz = 1 << 16;
        static thread_local char* buffer = new char[sz];

        const auto path = get_server_path(req->path());

        std::ifstream fs(path);
        
        helloworld::File reply;
        Stat stat;
        set_stat(path.c_str(), &stat);
        int status = (int)FileStatus::OK;
        if (stat.mtim() == req->ts()) {
            status = ((int)FileStatus::FILE_ALREADY_CACHED);
        } else if (!fs.good()) {
            status = ((int)FileStatus::FILE_OPEN_ERROR);
        }

        std::cerr << "streaming status = " << status << "\n";
        reply.set_status(status);
        writer->Write(reply);
        if (status == (int)FileStatus::OK)
            std::cerr << "streaming file now\n";
            while (!fs.eof()) {
                fs.read(buffer, sz);
                reply.set_byte(buffer);
                writer->Write(reply);
            }
        return Status::OK;
    }

    Status s_release(ServerContext* context, ServerReader<helloworld::File>* reader, Int* reply) override {
        helloworld::File file;
        reader->Read(&file);
        const auto path = get_server_path(file.path());
        std::ofstream fs(path);
        
        while (reader->Read(&file)) {
            fs << file.byte();
        }
        fs.close();
        return Status::OK;
    }
    
    Status s_access(ServerContext* context, const PathNFlag* req, Int* reply) override {
        int res = ::access(req->path().c_str(),req->flag());
        if (res == -1){
            reply->set_value(-errno);
        }
        else{
            reply->set_value(0);
        }
        return Status::OK;
}
private:
    static void set_stat(const char* path, Stat* reply) {
        const auto stat = get_stat(path);
        set_time(reply->mutable_atim(), stat.st_atim);
        set_time(reply->mutable_mtim(), stat.st_mtim);
        set_time(reply->mutable_ctim(), stat.st_ctim);

        reply->set_ino(stat.st_ino);
        reply->set_mode(stat.st_mode);
        reply->set_nlink(stat.st_nlink);
        reply->set_uid(stat.st_uid);
        reply->set_gid(stat.st_gid);
        reply->set_rdev(stat.st_rdev);
        reply->set_size(stat.st_size);
        reply->set_blocks(stat.st_blocks);
    }

    static void set_time(helloworld::Time* ret, const struct timespec& ts) {
        ret->set_sec(ts.tv_sec);
        ret->set_nsec(ts.tv_nsec);
    }

    static struct stat get_stat(const char* path) {
        struct stat st;
        ::stat(path, &st);
        return st;
    }
    
};

void sigintHandler(int sig_num)
{
    std::cerr << "Clean Shutdown\n";
    fflush(stdout);
    std::exit(0);
}

void run_server()
{
    std::string server_address("localhost:" + get_port_from_env());
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
int main()
{

    // "ctrl-C handler"
    signal(SIGINT, sigintHandler);


    run_server();
}
