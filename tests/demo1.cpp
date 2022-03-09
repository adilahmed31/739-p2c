#include <bits/stdc++.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <dirent.h>
#include <utime.h>

std::fstream fs;
void sigintHandler(int sig_num)
{
    std::cerr << "Clean Shutdown\n";
    std::exit(0);
}



int main() {
    signal(SIGINT, sigintHandler);

    fs.open("a.txt", std::fstream::out);
    if (!fs.good()) {
        std::cerr << "fopen issue\n";
    }
    std::cerr << "write: " << "something new" << "\n";
    fs <<  "something new" << "\n";
    fs.flush();
    fs.close();
    while (1) usleep(1e5);

}
