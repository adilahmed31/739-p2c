#include "client.h"
#include <future>
#define CLOSE_DIRTY_OPT
#include <fuse.h>
#include <sys/stat.h>
#include <unistd.h>

static struct options {
	const char *host;
	const char *port;
	int show_help;
} options;

#define OPTION(t,p){t, offsetof(struct options, p), 1}

static const struct fuse_opt option_spec[] = {
    OPTION("--host=%s", host),
    OPTION("--port=%s", port),
    OPTION("-h", show_help),
    OPTION("--help",show_help),
    FUSE_OPT_END
};

static fd_data* fds = BasicRPCClient::fds;
fd_data BasicRPCClient::fds[1 << 16];
void sigintHandler(int sig_num)
{
    std::cerr << "Clean Shutdown\n";
    //    if (srv_ptr) {
    //        delete srv_ptr;
    //    }
    fflush(stdout);
    std::exit(0);
}
int do_fsync(const char* path, int, struct fuse_file_info* fi);

template <class T>
auto get_time(const T& t) {
    struct timespec ret;
    ret.tv_nsec = 0;
    if constexpr (std::is_same_v<T, uint64_t>) {
        ret.tv_sec = t;
    } else {
        ret.tv_sec = t.sec();
        ret.tv_nsec = t.nsec();
    }
    return ret;
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
    st->st_atim = get_time(s.atim());
    st->st_mtim = get_time(s.mtim());
		
    return s.error();
}

int do_fgetattr(const char* path, struct stat* stbuf,  fuse_file_info*) {
    //std::cerr << __PRETTY_FUNCTION__ << '\n';
    return do_getattr(path, stbuf);
}
static void *hello_init(struct fuse_conn_info *conn)
{
    //std::cerr << __PRETTY_FUNCTION__ << std::endl;
	return NULL;
}

static int do_open(const char* path, struct fuse_file_info* fi) {
    //std::cerr << __PRETTY_FUNCTION__ << '\n';
    fi->fh = greeter->c_open(path, fi->flags);
#ifdef PARALLEL_THREAD_OPT
    fds[fi->fh].size = ret.size;
    std::cerr << "PARALLEL_THREAD_OPT size = " << ret.size << "\n";
#endif
#ifdef CLOSE_DIRTY_OPT
    fds[fi->fh].dirty = 0;
#endif
    return 0;
}
int do_mkdir(const char* path, mode_t mode) {
    return greeter->c_mkdir(path);
}
int do_rmdir(const char* path) {
    return greeter->c_rmdir(path);
}

static int do_create(const char* path, mode_t mode, struct fuse_file_info* fi){
     //std::cerr << __PRETTY_FUNCTION__ << '\n';
     if (int ret = greeter->c_create(path, fi->flags); ret < 0)
        return ret;
    const int fd = fi->fh = greeter->c_open(path, fi->flags);
    if (fd < 0) return fd;
    return 0;
}

static int do_access(const char* path, int) {
    //std::cerr << __PRETTY_FUNCTION__ << '\n';
    return 0;
}

static int do_read(const char* path, char* buf,
    size_t size, off_t offset, struct  fuse_file_info *fi){
#ifdef SMALL_READ_OPT
    if (offset == 0 && size == 4096) {
        const auto& fd = fds[fi->fh];
        if (fd.buffer && fd.fd == fi->fh)
            memcpy(buf, fd.buffer, fd.buf_size); // TODO::
    {
#endif
    int rc = 0;
    rc = pread(fi->fh,buf,size,offset);
    
    if(rc<0){
        return -errno;
    }

    return rc;
}

static int do_flush(const char* path, struct fuse_file_info* fi) ;

static int do_write(const char* path, const char* buf,
        size_t size, off_t offset, struct  fuse_file_info *fi){
    const int rc = pwrite(fi->fh,buf,size,offset);
//    std::cerr << __PRETTY_FUNCTION__ << path << " " << rc << "\n";
    if(rc<0){
        return -errno;
    }
#ifdef CLOSE_DIRTY_OPT
    fds[fi->fh].dirty = 1;
#endif
    if (RUN_MODE == RUN_MODE_ENUM::SYNC) {
        do_fsync(path, 0, fi);
    }
    return rc;
}

static int do_opendir(const char* path, struct fuse_file_info* fi){
    //std::cerr << __PRETTY_FUNCTION__ << '\n';
    return 0;
}

static int do_releasedir(const char* path, struct fuse_file_info* fi){
    //std::cerr << __PRETTY_FUNCTION__ << '\n';
    return 0;
}

int do_unlink(const char* path) {
    return greeter->c_unlink(path);
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

int do_fsync(const char* path, int, struct fuse_file_info* fi) {
#ifdef CLOSE_DIRTY_OPT
    bool skip_grpc = false;
    if (!fds[fi->fh].dirty) {
        const Stat s = greeter->c_stat(path);
        if (s.mtim() == get_mod_ts(get_cache_path(path).c_str())) {
            //std::cerr << path << " File is clean. skip gRPC\n";
            skip_grpc = true;
        }
        else 
            ;//std::cerr << path << " updated on server. overwriting with our version!\n";
    }
    if (!skip_grpc)
#endif
    return greeter->c_flush(path, fi->fh);
    return 0;
}

static int do_flush(const char* path, struct fuse_file_info* fi) {
    if (RUN_MODE == RUN_MODE_ENUM::SYNC)
        return 0;
    return do_fsync(path, 0, fi);
}

static int do_release(const char* path, struct fuse_file_info* fi) {
    return ::close(fi->fh);
}
void test() {
    usleep(1e6);
    const char* fname = "/tmp/ab_fs/b.txt";
    int fd = ::open(fname, O_CREAT| O_RDWR);
//    std::cerr << "open w fd:"  << fd << "\n";
    ::write(fd, fname, strlen(fname));
    ::write(fd, fname, strlen(fname));
    ::write(fd, "\n", 1);
    ::close(fd);
}

static void show_help(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
	       "    --host=<s>          Hostname of the remote server"
	       "                        (default: localhost)\n"
	       "    --port=<s>          Port on which the server is running "
	       "                        (default is taken from GRPC environment variable"
	       "\n");
}





static struct fuse_operations operations;
int main(int argc, char *argv[])
{
     // "ctrl-C handler"
    signal(SIGINT, sigintHandler);
    
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    //Set defaults
    options.host = strdup("localhost");
    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

    if (options.show_help) {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    }
    
    std::string host = options.host;
    std::string port = options.port;
    
    const std::string target_str = host + port;
    grpc::ChannelArguments ch_args;

    ch_args.SetMaxReceiveMessageSize(INT_MAX);
    ch_args.SetMaxSendMessageSize(INT_MAX);

    greeter = std::make_unique<BasicRPCClient>(
            grpc::CreateCustomChannel(target_str,
            grpc::InsecureChannelCredentials() , ch_args ));

    auto tester = std::async(std::launch::async, [&]() { test(); });
    operations.create = do_create;
    operations.init = hello_init;
    operations.open = do_open;
    operations.getattr = do_getattr;
    operations.readdir = do_readdir;
//    operations.access = do_access;
    operations.read = do_read;
    operations.write = do_write;
    operations.opendir = do_opendir;
    operations.readdir = do_readdir;
    operations.releasedir = do_releasedir;
//    operations.fgetattr = do_fgetattr;
    operations.release = do_release;
    operations.fsync = do_fsync;
    operations.flush = do_flush;
    operations.mkdir = do_mkdir;
    operations.rmdir = do_rmdir;
    operations.unlink = do_unlink;
    int ret =  fuse_main(args.argc, args.argv, &operations, &greeter);
    fuse_opt_free_args(&args);
    return ret;
}
