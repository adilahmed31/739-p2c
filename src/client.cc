#include "client.h"

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

void sigintHandler(int sig_num)
{
    std::cerr << "Clean Shutdown\n";
    //    if (srv_ptr) {
    //        delete srv_ptr;
    //    }
    fflush(stdout);
    std::exit(0);
}
std::unique_ptr<BasicRPCClient> greeter;
#include <sys/stat.h>
#include <unistd.h>
int do_getattr(const char* path, struct stat* st) {
    const Stat s = greeter->c_stat(path);
    st->st_ino = s.ino();
    st->st_mode = s.mode();
    st->st_nlink = s.nlink();
    st->st_uid = s.uid();
    st->st_gid = s.gid();
    st->st_rdev = s.rdev();
    st->st_size = s.size();
    st->st_blocks = s.blocks();
    auto get_time = [&](const helloworld::Time& t) {
        struct timespec ret;
        ret.tv_sec = t.sec();
        ret.tv_nsec = t.nsec();
        return ret;
    };
    st->st_atim = get_time(s.atim());
    st->st_mtim = get_time(s.mtim());
    st->st_ctim = get_time(s.ctim());
    return 0;
}
#include <fuse.h>

static void *hello_init(struct fuse_conn_info *conn)
{
    std::cerr << __PRETTY_FUNCTION__ << std::endl;
	return NULL;
}

static int do_open(const char* path, struct fuse_file_info* fi) {
    std::cerr << __PRETTY_FUNCTION__ << '\n';
}

static int do_access(const char* path, int) {
    std::cerr << __PRETTY_FUNCTION__ << '\n';
    return 0;
}


int do_readdir(const char* path, void* buffer, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info* fi) {
    const auto resp = greeter->c_readdir(path);
    if (auto err = resp.ret_code(); err < 0)
        return err;

    for (auto &s : resp.names()) {
        filler(buffer, s.c_str(), nullptr, 0);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // "ctrl-C handler"
    signal(SIGINT, sigintHandler);
    const std::string target_str = "localhost:50051";
    grpc::ChannelArguments ch_args;

    ch_args.SetMaxReceiveMessageSize(INT_MAX);
    ch_args.SetMaxSendMessageSize(INT_MAX);

    greeter = std::make_unique<BasicRPCClient>(
            grpc::CreateCustomChannel(target_str,
            grpc::InsecureChannelCredentials() , ch_args ));

    greeter->c_create("/tmp/a.txt", 0);
    print_proto_stat(greeter->c_stat("/tmp/a.txt"));

    struct fuse_operations operations;
    operations.init = hello_init;
    operations.getattr = do_getattr;
    operations.readdir = do_readdir;
    operations.access = do_access;

    return fuse_main(argc, argv, &operations, &greeter);
}
