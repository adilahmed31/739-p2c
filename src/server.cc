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
#include <utime.h>

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
    Stats st_creat, st_mkdir, st_stat, st_open,
            st_close;
public:
    BasicRPCServiceImpl():
            st_creat("server_create"),
            st_mkdir("server_mkdir"),
            st_stat("server_stat"),
            st_open("server_open"),
            st_close("server_close") {}
    Status s_unlink(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_serv_calls(__PRETTY_FUNCTION__);
        const auto path = get_server_path(req->path());
        reply->set_value(0);
        if ( ::unlink(path.c_str()) < 0 )
            reply->set_value(-errno);
   //     std::ofstream fs(path.c_str()); fs << "test string";
        return Status::OK;
    }


    Status s_creat(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_serv_calls(__PRETTY_FUNCTION__);
        Clocker _(st_creat);
        const auto path = get_server_path(req->path());
        reply->set_value(0);
        int fd;
        if ( (fd=::creat(path.c_str(), 0777)) < 0 )
            reply->set_value(-errno);
        else ::close(fd);
   //     std::ofstream fs(path.c_str()); fs << "test string";
        struct stat  st ;
        const int ret = get_stat(path.c_str(), st);
        reply->set_ts(st.st_mtim.tv_sec);
        return Status::OK;
    }

    Status s_mkdir(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        Clocker _(st_mkdir);
        cerr_serv_calls(__PRETTY_FUNCTION__);
        const auto path = get_server_path(req->path());
        if(::mkdir(path.c_str(), 0777) < 0) { 
            reply->set_value(-errno);
        }
        struct stat  st ;
        const int ret = get_stat(path.c_str(), st);
        reply->set_ts(st.st_mtim.tv_sec);
        return Status::OK;
    }

    Status s_rmdir(ServerContext* context, const PathNFlag* req
                           , Int* reply) override
    {
        cerr_serv_calls(__PRETTY_FUNCTION__);
        const auto path = get_server_path(req->path());
        if(::rmdir(path.c_str()) < 0) { 
            reply->set_value(-errno);
        }

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
        cerr_serv_calls(__PRETTY_FUNCTION__);
        DIR *dp;
        struct dirent *de;
        const auto path = get_server_path(req->path());
        cerr_serv_calls("s_readdir on ", path);
        dp = opendir(path.c_str());
        reply->set_ret_code(0);
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
        cerr_serv_calls(__PRETTY_FUNCTION__);
        Clocker _(st_stat);
        const auto path = get_server_path(req->path());
        const int ret = set_stat(path.c_str(), reply);
        reply->set_error(ret);
//        cerr_serv_calls("[*] stat ", path,
//            " sz = ", reply->size(), " mode = ", reply->mode());
        return Status::OK;
    }

    Status s_open(ServerContext* context, const PathNFlag* req, 
                    ServerWriter<helloworld::File>* writer) override {

        cerr_serv_calls(__PRETTY_FUNCTION__);
        Clocker _(st_open);
        constexpr int sz = 1 << 16;
        static thread_local char* buf= new char[sz];

        const auto path = get_server_path(req->path());

        const int fd = ::open(path.c_str(), O_RDWR);
        const int err = (fd < 0) ? -errno : 0;
        helloworld::File reply;
        Stat stat;
        const int ret = set_stat(path.c_str(), &stat);
        int status = (int)FileStatus::OK;
        if (stat.mtim() == req->ts() && stat.mtim() != 0 && ret == 0) {
            status = ((int)FileStatus::FILE_ALREADY_CACHED);
        } else if (fd < 0) {
            status = ((int)FileStatus::FILE_OPEN_ERROR);
            cerr_serv_calls("server errno = ", err," for file: ", path);
        }
     //   std::cerr << "streaming status = " << status << " @ "
     //           << path << " " 
     //           << stat.mtim() << " " << req->ts()  << "\n";
        reply.set_error(err);
        reply.set_mtim(stat.mtim());
        reply.set_status(status);
        writer->Write(reply);
        if (status == (int)FileStatus::OK) {
            int n;
            int tot_sent = 0;
            while (n = ::read(fd, buf, sz)) {
                std::string tmp = std::string(buf, n);
                reply.set_byte(std::move(tmp));
                tot_sent += n;
                writer->Write(reply);
            }
            //std::cerr << "[open] streamed: " << tot_sent << " bytes\n";
        }
        ::close(fd);
        return Status::OK;
    }

    Status s_close(ServerContext* context, ServerReader<
                  helloworld::File>* reader, Int* reply) override {
        cerr_serv_calls(__PRETTY_FUNCTION__);
        Clocker _(st_close);
        helloworld::File file;
        reader->Read(&file);
        struct utimbuf new_ts;
        new_ts.modtime = file.mtim();

        const auto path = get_server_path(file.path());
        auto [tmp_fd, tmp_fname] = get_tmp_file();
        cerr_serv_calls("closing file: ", file.path(), " saving to tmp:", tmp_fname,
            " modts: ", new_ts.modtime);

        while (reader->Read(&file)) {
            ::write(tmp_fd, file.byte().c_str(), file.byte().length());
        }
        ::close(tmp_fd);
        ::utime(tmp_fname.c_str(), &new_ts);
        ::rename(tmp_fname.c_str(), path.c_str());
        ::utime(path.c_str(), &new_ts);

        return Status::OK;
    }

private:
    static int set_stat(const char* path, Stat* reply) {
        struct stat stat;
        const auto ret = get_stat(path, stat);
        if (ret !=0) return ret;
        set_time(reply->mutable_atim(), stat.st_atim);
        reply->set_mtim(stat.st_mtim.tv_sec);
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

void run_server(std::string hostname, std::string port_number)
{
    std::string server_address(hostname +":"+ port_number);
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

int main(int argc, char* argv[])
{
 // "ctrl-C handler"
    signal(SIGINT, sigintHandler);
    std::string host, port;
    if (argc == 1){
	host = "localhost";
        port = get_port_from_env();
    }
    else if (argc == 2){
        host = argv[1];
    	port = get_port_from_env();
    } 
    else{
	host = argv[1];
	port = argv[2];
    }

    //Create server path if it doesn't exist
    DIR* dir = opendir(get_server_path("").c_str());
    if (ENOENT == errno){
        mkdir(get_server_path("").c_str(),0777);
    }
    run_server(host,port);
}
