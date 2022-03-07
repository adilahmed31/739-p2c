#pragma once
#include <bits/stdc++.h>
class Stats {
    std::vector<uint64_t> stats;
    std::string name;
public:
    Stats(): name("unnnamed"){}
    Stats(const std::string name_): name(std::move(name_)) {}
    void add(uint64_t ns) {
        stats.push_back(ns);
    }
    ~Stats() {
        std::cout << name << ", ";
        if (stats.size() == 0) {
            std::cout << "weird...\n";
        }
        if (stats.size() == 1) {
            std::cout << stats.front() << std::endl;
        } else {
            const auto sum = std::accumulate(stats.begin(), stats.end(), 0ULL);
            std::cout <<(int) ( ((double) sum) / stats.size() ) << std::endl;
        }
    }
};

struct Clocker {
    const std::chrono::time_point<std::chrono::high_resolution_clock> start;
    Stats& stats;
    Clocker(Stats& stat): start(std::chrono::high_resolution_clock::now()), stats(stat){}
    uint64_t get_ns() const {
        using namespace std::chrono;
        return duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
    }
    ~Clocker() {
        stats.add(get_ns());
    }
};


