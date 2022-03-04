#include "client.h"
#include <future>

#include <fuse.h>
#include <sys/stat.h>
#include <unistd.h>

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

static void *hello_init(struct fuse_conn_info *conn)
{
    std::cerr << __PRETTY_FUNCTION__ << std::endl;
	return NULL;
}

static int do_open(const char* path, struct fuse_file_info* fi) {
    std::cerr << __PRETTY_FUNCTION__ << '\n';
    fi->fh = 1;
    return 1;
}


int do_opendir(const char *, struct fuse_file_info *) {
    return 0;
}


static int do_access(const char* path, int) {
    std::cerr << __PRETTY_FUNCTION__ << '\n';
    return 0;
}

static int do_read(const char* path, char* buf, size_t size, off_t offset, struct  fuse_file_info *fi){
    int rc = 0;

    rc = pread(fi->fh,buf,size,offset);
    if(rc<0){
        return -errno;
    }

    return rc;
}

static int do_write(const char* path, const char* buf, size_t size, off_t offset, struct  fuse_file_info *fi){
    int rc = 0;

    rc = pwrite(fi->fh,buf,size,offset);
    if(rc<0){
        return -errno;
    }

    return rc;
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

void test() {
    usleep(1e6);
    greeter->c_create("/tmp/a.txt", 0777);
    print_proto_stat(greeter->c_stat("/tmp/a.txt"));

    std::cerr << "trying to open fuse file /tmp/ab_fuse/a.txt\n";
    std::ifstream fs("/tmp/ab_fuse/a.txt");
    if (!fs.good()) std::cerr << "file open failed\n";
    char buf[100];
    while (!fs.eof()) {
        fs.read(buf, sizeof(buf));
        std::cerr << buf;
    }
}
int main(int argc, char *argv[])
{
    // "ctrl-C handler"
    signal(SIGINT, sigintHandler);
    const std::string target_str = "localhost:" + get_port_from_env();
    grpc::ChannelArguments ch_args;

    ch_args.SetMaxReceiveMessageSize(INT_MAX);
    ch_args.SetMaxSendMessageSize(INT_MAX);

    greeter = std::make_unique<BasicRPCClient>(
            grpc::CreateCustomChannel(target_str,
            grpc::InsecureChannelCredentials() , ch_args ));
    auto tester = std::async(std::launch::async, [&]() { test(); });
//    struct fuse_file_info *fi = new struct fuse_file_info();
//    fi->fh = greeter->c_open("/tmp/a.txt", O_RDWR);
//    char readbuf[100];
//    if (fi->fh <0){
//        std::cout << "Open error!"<<std::endl;
//    }
//    do_read(NULL,readbuf,100, 0,fi);
//    std::cout << readbuf <<std::endl;;
    struct fuse_operations operations;
    operations.init = hello_init;
    operations.getattr = do_getattr;
    operations.readdir = do_readdir;
    operations.access = do_access;
    operations.read = do_read;
    operations.write = do_write;
    operations.opendir = do_opendir;
    return fuse_main(argc, argv, &operations, &greeter);
}
