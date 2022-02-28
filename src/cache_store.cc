#include "cache_store.h"


#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
namespace Cache {
std::string CacheStore::get_home() {
    return getpwuid(getuid())->pw_dir;
}
}
