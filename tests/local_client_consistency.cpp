#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    std::string fs_path = "/tmp/ab_fs/";
    std::string path = fs_path + "a.txt";

    int fd1 = ::open(path.c_str(), O_RDWR | O_CREAT, "w");

    ::write(fd1, fs_path.c_str(), fs_path.length());

    int fd2 = ::open(path.c_str(), O_RDWR, "r");

    char buf[100];
    int n;
    while ((n = ::read(fd2, buf, sizeof(buf))) > 0) {
        std::cout << "read: " << std::string(buf, n) << "\n";
    }

    std::cerr << "waiting on inf loop without closing fd\n";
    while (1) usleep(1e6);
}
