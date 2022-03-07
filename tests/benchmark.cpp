// rm -rf ~/.fuse_server/* ; g++ benchmark.cpp -std=c++17 -O3 -pthread &&   ./a.out
// rm -rf ~/.fuse_server/* ; g++ benchmark.cpp -std=c++17 -O3 -pthread &&   ./a.out
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "helper.h"
const std::string fs_path = "/tmp/ab_fs/";
 
void bench_create()
{
    const int ITERS = 1e3;
    std::vector<int> fds;
    fds.reserve(ITERS);

    std::vector<std::string> fnames(ITERS);
    for (int i = 0; i < ITERS; i++) {
        fnames[i] = fs_path + std::to_string(i);
    }

    //if (std::getenv("CONLY")) {
        Stats st_creat("client_create");
        Stats st_cteat1("1000_create_time_1_thread");
        {
        Clocker c_st_cteat1(st_cteat1);
        for (int i = 0; i < ITERS; i++) {
            Clocker _(st_creat);
            fds.push_back(::open(fnames[i].c_str(), O_CREAT, "w"));
        }
        }
        {
            Stats st_fsync_no_change("client_fsync_no_change");
            for (int i = 0; i < ITERS; i++) {
                Clocker _(st_fsync_no_change);
                ::fsync(fds[i]);
            }
        }
        {
            Stats st_close_em("client_close_empty");
            for (int i = 0; i < ITERS; i++) {
                Clocker _(st_close_em);
                ::close(fds[i]);
            }
        }
        {
            Stats st_remove("client_remove");
            for (int i = 0; i < ITERS; i++) {
                Clocker _(st_remove);
                if (::unlink(fnames[i].c_str()) < 0) {
                  //  std::cerr << "Error in remove: " << errno << "\n";
                }
            }
        }

    {
        Stats st_creat4("1000_create_time_4_threads");
        Clocker c_st_creat4(st_creat4);
        const int blocks = ITERS/4;
        std::vector<std::thread> ths;
        for (int i = 0 ; i< 4; i++) {
            auto runnable = [i, blocks, &fds, &fnames]() {
                const int st = i * blocks;
                for (int j = st; j < st + blocks; j++)
                    fds.push_back(::open(fnames[j].c_str(), O_CREAT, "w"));   
            };
            ths.emplace_back(runnable);
        }
        std::for_each(ths.begin(), ths.end(), [](std::thread &t) 
        {
            t.join();
        });

    }
}

void bench_open_first() {
    const std::vector<double> sizes = { 1e2, 1e3, 1e4, 1e5, 5e5, 1e6, 5e6};
    const std::vector<int> iters = { 100, 50, 10, 3, 1};

    const int N = sizes.size();
    std::vector<std::string> fnames(N);

    for (int i = 0; i < N; i++) {
        fnames[i] = fs_path + std::to_string(i);
    }
    const int max_sz = *std::max_element(sizes.begin(), sizes.end());
    static thread_local char* buf =
        new char[max_sz];
    for (int i = 0; i < N; i++) {
        int fd = ::open(fnames[i].c_str(), O_CREAT | O_RDWR, "w");
        if (fd < 0) {
            std::cerr << "bad fd: " << fd << " " << errno << "\n";
        }
        const int sz = sizes[i];
        std::fill(buf, buf + sz, 'X');
        Stats write_sz("client_write_empty," + std::to_string(sz) );
        {
            Clocker _(write_sz);
            int ret;
            if ((ret = ::write(fd, buf, sz)) != sz)
                std::cerr << "error in write: " << sz << " " << ret << " " << errno<< "\n";
        }
        ::close(fd);
    }
    for (int i = 0; i < N; i++) {
        int fd = ::open(fnames[i].c_str(), O_CREAT | O_RDWR, "w");
        if (fd < 0) {
            std::cerr << "bad fd: " << fd << " " << errno << "\n";
        }
        const int sz = sizes[i];
        Stats read_sz("client_read," + std::to_string(sz));
        {
            Clocker _(read_sz);
            int ret;
            if ((ret = ::read(fd, buf, sz)) != sz)
                std::cerr << "error in read: " << sz << " " << ret << " " << errno<< "\n";
        }
        ::close(fd);
    }
    system("rm -f /tmp/fuse_cache/*");
    Stats st_close("client_clean_file_close");
    for (int i = 0; i < N; i++) {
        const int sz = sizes[i];
        Stats open_sz("client_first_open_" + std::to_string(sz) + "B");
        int fd;
        {
            Clocker _(open_sz);
            fd = ::open(fnames[i].c_str(), O_RDWR, "r");
        }
        Clocker _(st_close);
        ::close(fd);
    }
}
const bool SYNC_MODE = []() {
        const auto mode = std::getenv("RUN_MODE");
        if (mode == nullptr) {
            return false;
        }
        if (strcmp(mode, "STRICT") == 0) {
            return false;
        }
        if (strcmp(mode, "SYNC") == 0) {
            std::cerr << "[RUN_MODE] = SYNC\n";
            return true;
        }
        return true;
}();


void rw_th() {
    // write 100MB using different block sizes
    const std::vector<int> sizes = {100, 1000, 10000, 100000};
    const uint64_t TOTAL_WRITE = 100 * (1000 * 1000) / (SYNC_MODE ? 10 : 1); // 100 MB
    const int max_sz = *std::max_element(sizes.begin(), sizes.end());
    char* buf = new char[max_sz];
    std::fill(buf, buf + max_sz, 'Z');
    for (auto sz:sizes) {
        const auto fname = fs_path + std::to_string(sz);
        int fd = ::open(fname.c_str(), O_CREAT | O_RDWR, "w");
        const int target_write = SYNC_MODE ? (sz * 50) : TOTAL_WRITE;
        if (fd < 0) { std::cerr << "rw_th ::open error\n"; }
        {
            Stats write_ts_sz("write_ts, " + std::to_string(sz)
                        + ", " + std::to_string(target_write));
            Clocker _(write_ts_sz);
            uint64_t written = 0;
            while (written < target_write) {
                ::write(fd, buf, sz);
                written += sz;
            }
        }
        ::close(fd);
        fd = ::open(fname.c_str(), O_RDWR, "r");
        if (fd < 0) { std::cerr << "rw_th ::open error\n"; }
        {
            Stats write_ts_sz("read_ts, " + std::to_string(sz)
                    + ", " + std::to_string(target_write));
            Clocker _(write_ts_sz);
            uint64_t readd = 0;
            int r1;
            while (r1 = ::read(fd, buf, sz)) {
                readd += r1;
            }
            if (readd != target_write) { std::cerr << "issue in read\n"; }
        }
        ::close(fd);
        ::remove(fname.c_str());
    }

    delete[] buf;
}

int main() {
    rw_th();
    bench_open_first();
    bench_create();
}

