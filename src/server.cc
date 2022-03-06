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
        reply->set_value(0);
        if ( ::creat(path.c_str(), 0777) < 0 )
            reply->set_value(-errno);
   //     std::ofstream fs(path.c_str()); fs << "test string";
        struct stat  st ;
        const int ret = get_stat(path.c_str(), st);
        set_time(reply->mutable_ts(), st.st_mtim);
        return Status::OK;
    }

    Status s_mkdir(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_serv_calls(__PRETTY_FUNCTION__);
        const auto path = get_server_path(req->path());
        reply->set_value(::mkdir(path.c_str(), req->flag()));
        struct stat  st ;
        const int ret = get_stat(path.c_str(), st);
        set_time(reply->mutable_ts(), st.st_mtim);
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
        const int ret = set_stat(path.c_str(), reply);
        reply->set_error(ret);
        cerr_serv_calls(__PRETTY_FUNCTION__, " -> ", path,
            " sz = ", reply->size());
        return Status::OK;
    }

    Status s_open(ServerContext* context, const PathNFlag* req, 
                    ServerWriter<helloworld::File>* writer) override {
        cerr_serv_calls(__PRETTY_FUNCTION__);
        constexpr int sz = 1 << 16;
        static thread_local char* buf= new char[sz];

        const auto path = get_server_path(req->path());

        const int fd = ::open(path.c_str(), O_RDWR);
        const int err = (fd < 0) ? -errno : 0;
        helloworld::File reply;
        Stat stat;
        const int ret = set_stat(path.c_str(), &stat);
        int status = (int)FileStatus::OK;
        if (stat.mtim() == req->ts()) {
            status = ((int)FileStatus::FILE_ALREADY_CACHED);
        } else if (fd < 0) {
            status = ((int)FileStatus::FILE_OPEN_ERROR);
            cerr_serv_calls("server errno = ", err," for file: ", path);
        }
        std::cerr << "streaming status = " << status << "\n";
        reply.set_error(err);
        reply.set_status(status);
        writer->Write(reply);
        if (status == (int)FileStatus::OK) {
            int n;
            int tot_sent = 0;
            while (n = ::read(fd, buf, sz)) {
                std::string tmp = std::string(buf, n);
                std::cerr << n << " -> " << tmp << "\n";
                reply.set_byte(std::move(tmp));
                tot_sent += n;
                writer->Write(reply);
            }
            std::cerr << "[open] streamed: " << tot_sent << " bytes\n";
        }
        return Status::OK;
    }

    Status s_close(ServerContext* context, ServerReader<
                  helloworld::File>* reader, Int* reply) override {
        helloworld::File file;
        reader->Read(&file);
        const auto path = get_server_path(file.path());
        std::ofstream fs(path);
        cerr_serv_calls("closing file: ", file.path());
        while (reader->Read(&file)) {
            fs << file.byte();
        }
        return Status::OK;
    }

private:
    static int set_stat(const char* path, Stat* reply) {
        struct stat stat;
        const auto ret = get_stat(path, stat);
        if (ret !=0) return ret;
        set_time(reply->mutable_atim(), stat.st_atim);
        set_time(reply->mutable_mtim(), stat.st_mtim);
        set_time(reply->mutable_ctim(), stat.st_ctim);
        reply->set_size(stat.st_size);

        reply->set_ino(stat.st_ino);
        reply->set_mode(stat.st_mode);
        reply->set_nlink(stat.st_nlink);
        reply->set_uid(stat.st_uid);
        reply->set_gid(stat.st_gid);
        reply->set_rdev(stat.st_rdev);
        reply->set_size(stat.st_size);
        reply->set_blocks(stat.st_blocks);
        return 0;
    }

    static void set_time(helloworld::Time* ret, const struct timespec& ts) {
        ret->set_sec(ts.tv_sec);
        ret->set_nsec(ts.tv_nsec);
    }

    static int get_stat(const char* path, struct stat& st) {
        const int ret = ::stat(path, &st);
        if (ret < 0) { cerr_serv_calls("stat got error for ", path, " -> ", -errno);  return -errno; }
        return 0;
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
