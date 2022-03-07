#pragma once
#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 35


#include "helloworld.grpc.pb.h"
#include <utime.h>
#include <sys/stat.h>
#include <numeric>

enum class RUN_MODE_ENUM {
    POSIX, SYNC, STRICT
};

class Stats {
    std::vector<uint64_t> stats;
    std::string name;
public:
    Stats(): name("unnnamed"){}
    Stats(const std::string name_): name(std::move(name_)) {}
    void add(uint64_t ns) {
#ifndef DISBALE_BENCH
        stats.push_back(ns);
#endif
    }
    ~Stats() {
#ifndef DISBALE_BENCH
        std::cout << name << ", ";
        if (stats.size() == 0) {
            std::cout << "X\n";
        } else if (stats.size() == 1) {
            std::cout << stats.front() << "\n";
        } else {
            const auto sum = std::accumulate(stats.begin(), stats.end(), 0ULL);
            std::cout <<(int) ( ((double) sum) / stats.size() ) << "\n";
        }
#endif
    }
};

struct Clocker {
    const std::chrono::time_point<std::chrono::high_resolution_clock> start;
    Stats& stats;
    Clocker(Stats& stat): start(std::chrono::high_resolution_clock::now()), stats(stat){}
    uint64_t get_ns() const {
#ifndef DISBALE_BENCH
        using namespace std::chrono;
        return duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
#endif
        return 0;
    }
    ~Clocker() {
#ifndef DISBALE_BENCH
        stats.add(get_ns());
#endif
    }
};


const auto RUN_MODE = []() {
        const auto mode = std::getenv("RUN_MODE");
        if (mode == nullptr) {
            std::cerr << "[RUN_MODE] = NULL. using POSIX\n";
            return RUN_MODE_ENUM::POSIX;
        }
        if (strcmp(mode, "STRICT") == 0) {
            std::cerr << "[RUN_MODE] = STRICT\n";
            return RUN_MODE_ENUM::POSIX;
        }
        if (strcmp(mode, "SYNC") == 0) {
            std::cerr << "[RUN_MODE] = SYNC\n";
            return RUN_MODE_ENUM::SYNC;
        }
        std::cerr << "[RUN_MODE] = POSIX\n";
        return RUN_MODE_ENUM::POSIX;
}();

using helloworld::BasicRPC;
using helloworld::Int;
using helloworld::Stat;
using helloworld::PathNFlag;

namespace helloworld {
    inline bool operator==(const Time& a, const Time& b) {
        return a.sec() == b.sec() &&  a.nsec() == b.nsec();
    }
}

constexpr const char* CACHE_BASE_PATH = "/tmp/fuse_cache/";

inline std::string get_cache_path(const std::string path) {
    return std::string(CACHE_BASE_PATH) +
            std::to_string(std::hash<std::string>()(path));
}

inline std::string get_tmp_cache_path(const std::string path) {
    return std::string(CACHE_BASE_PATH) + ".tmp." +
             std::to_string(std::hash<std::string>()(path));
}

constexpr bool DISABLE_CERR_ERRORS = true;
constexpr bool CERR_SERVER_CALLS = false;
constexpr bool PRINT_SERVER_OUT = false;

inline std::pair<int, std::string> get_tmp_file() {
    char templat[100];
    strcpy(templat, "/users/agabhin/.fuse_server/afs_tmp_fileXXXXXX");
    const int fd = mkstemp(templat);
    ::chmod(templat, 0777);
    return {fd, std::string(templat)};
}

static uint64_t get_mod_ts(const char* path) {
    struct stat st;
    const int ret = ::stat(path, &st);
    if (ret < 0) st.st_mtim.tv_sec = 0;
    return st.st_mtim.tv_sec;
}

inline void print_ts(const helloworld::Time& ts) {
    std::cerr << "[" << ts.sec() << "." << ts.nsec() << "] ";
}
inline void print_ts(const uint64_t ts) {
    std::cerr << "[" << ts << "]\n";
}

inline void print_proto_stat(const Stat& st) {
    std::cerr << "ts: ";
    print_ts(st.atim());
    print_ts(st.mtim());
    print_ts(st.ctim());
    std::cerr << " " << st.blocks() << "\n";
}

template <class... T>
inline void cerr_errors(const T&... args) {
    if constexpr (!DISABLE_CERR_ERRORS)
        (std::cerr << ... << args) << '\n';
}

template <class... T>
inline void log_client(const T&... args) {
    if constexpr (!DISABLE_CERR_ERRORS)
        (std::cerr << ... << args) << '\n';
}


template <class ReplyT>
inline void print_server_out(const char* fn, const ReplyT& reply) {
    if constexpr (PRINT_SERVER_OUT)
        (std::cerr << fn << " -> " << reply.get_value() << "\n");
}


template <class... T>
inline void cerr_serv_calls(const T&... args) {
    if constexpr (CERR_SERVER_CALLS)
        (std::cerr << ... << args) << '\n';
}

enum class FileStatus: int {
    OK = 1,
    FILE_OPEN_ERROR,
    FILE_ALREADY_CACHED,
};

inline std::string get_port_from_env() {
    const char* ENV_VAR = "GRPC_PORT";
    const char* ret = std::getenv(ENV_VAR);
    if (ret == NULL) {
        std::cerr <<  ENV_VAR << " env var is not set. aborting\n";
        std::abort();
    }
    return std::string(std::getenv(ENV_VAR));
}
