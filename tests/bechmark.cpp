// rm ~/.fuse_server/* ; g++ bechmark.cpp -std=c++17 -O3 -pthread &&   ./a.out
// rm ~/.fuse_server/* ; g++ bechmark.cpp -std=c++17 -O3 -pthread &&   ./a.out
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

int main() {
    bench_open_first();
    bench_create();
}

