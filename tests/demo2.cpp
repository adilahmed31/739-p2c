#include <bits/stdc++.h>
#include <utime.h>
#include <unistd.h>
#include <fstream>

std::ofstream fs;
void sigintHandler(int sig_num)
{
    std::cerr << "file closed\n";
    fs.close();
    std::exit(0);
}



int main() {
    signal(SIGINT, sigintHandler);

    fs.open("a.txt");
    std::cerr << "a.txt opened\n";

    while (1) usleep(1e5);

}
