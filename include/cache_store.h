#pragma once
#include <string>
#include <cstring>

namespace Cache {
struct Time {
    uint64_t sec, nsec;
    Time(): sec(0), nsec(0) {}
    Time(uint64_t a, uint64_t b): sec(a), nsec(b) {}
};
 

struct FileInfo {
    static constexpr int SIZE = 256;
    char path[SIZE];
    Time ts;
    FileInfo() {
        ::bzero(path, SIZE);
    }
};


class CacheStore {
public:
    CacheStore(const char* path) {
        std::string fpath;
        if (strcmp(DEFAULT_CACHE_PATH, path) == 0) {
            fpath = get_home() + "/" +
                    std::string(DEFAULT_CACHE_PATH);
        } else {
            fpath = path;
        }
        // TODO: do mmaping here for reliability !!!
        mem = new FileInfo[100];
    }
    
    Time get_ts(const char* path) const {
        return Time();
    }
    
    void set_ts(const char* path, const Time& ts) {
        // TODO:
    } 
    void set_ts(const char* path, uint64_t a, uint64_t b) {
        set_ts(path, {a, b});
    } 
    
private:
    static std::string get_home();
    static constexpr const char* DEFAULT_CACHE_PATH
                = ".afs_cache.data";
    FileInfo* mem;
};
}
