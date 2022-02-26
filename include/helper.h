#pragma once

#include "helloworld.grpc.pb.h"


using helloworld::BasicRPC;
using helloworld::Int;
using helloworld::Stat;
using helloworld::PathNFlag;
using helloworld::Bytes;


void print_ts(const helloworld::Time& ts) {
    std::cerr << "[" << ts.sec() << "." << ts.nsec() << "] ";
}
void print_proto_stat(const Stat& st) {
    std::cerr << "ts: ";
    print_ts(st.atim());
    print_ts(st.mtim());
    print_ts(st.ctim());
}

