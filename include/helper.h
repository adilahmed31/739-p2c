#pragma once
#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 35


#include "helloworld.grpc.pb.h"


using helloworld::BasicRPC;
using helloworld::Int;
using helloworld::Stat;
using helloworld::PathNFlag;

namespace helloworld {
    bool operator==(const Time& a, const Time& b) {
        return a.sec() == b.sec() &&  a.nsec() == b.nsec();
    }
}

constexpr bool DISABLE_CERR_ERRORS = false;
constexpr bool PRINT_SERVER_OUT = true;

void print_ts(const helloworld::Time& ts) {
    std::cerr << "[" << ts.sec() << "." << ts.nsec() << "] ";
}
void print_proto_stat(const Stat& st) {
    std::cerr << "ts: ";
    print_ts(st.atim());
    print_ts(st.mtim());
    print_ts(st.ctim());
    std::cerr << " " << st.blocks() << "\n";
}

template <class... T>
void cerr_errors(const T&... args) {
    if constexpr (!DISABLE_CERR_ERRORS)
        (std::cerr << ... << args) << '\n';
}

template <class... T>
void log_client(const T&... args) {
    if constexpr (!DISABLE_CERR_ERRORS)
        (std::cerr << ... << args) << '\n';
}


template <class ReplyT>
void print_server_out(const char* fn, const ReplyT& reply) {
    if constexpr (PRINT_SERVER_OUT)
        (std::cerr << fn << " -> " << reply.get_value() << "\n");
}

constexpr bool CERR_SERVER_CALLS = true;

template <class... T>
void cerr_serv_calls(const T&... args) {
    if constexpr (CERR_SERVER_CALLS)
        (std::cerr << ... << args) << '\n';
}

enum class FileStatus: int {
    FILE_OPEN_ERROR,
    FILE_ALREADY_CACHED,
    OK
};
