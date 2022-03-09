# Not-so-simple Multi-mode Reliable Distributed File System (NssMRDFS)

## Overview

This file system was created as a class project for CS739 Distributed Systems at UW-Madison.
It implements a naive distributed file system that uses AFS-like semantics without the use of callbacks. The server supports most common file system operations such as
create, read, open, close, rm, readdir, mkdir, scandir, write, fsync etc. and excludes a permission or link based APIs. The client can be run in two modes:
1. POSIX mode - this is the default mode. Writes are buffered on the client and only flushed to the server on fsync() or close().
2. SYNC mode - this mode is to be used only  for cases where high reliability is required. Writes are flushed to the server immediately while offering strict consistency between client and server
in all crash scenarios. This mode runs slowly and is to be enabled manually by setting the environment variable RUN_MODE=SYNC while running the client.

The system uses gRPC for communication between the client and server. The FUSE interface is used to implement the filesystem client.

## Usage

### Code Compilation

````
git clone https://github.com/agarwal722/739-p2c.git
cd 739-p2c
mkdir build
cd build
cmake ../
make
````
### Run server
The server takes its own IP as an argument when run in distributed mode.
````
cd build/bin
./server <hostname> <port>
````
Example:
`./server 10.10.1.1 3000`

### Run client
````
cd build/bin
./client --help
File-system specific options:\n"
	       "    --host=<s>          Hostname of the remote server"
	       "                        (default: localhost)\n"
	       "    --port=<s>          Port on which the server is running "
	       "                        (default is taken from GRPC environment variable"
````
Example:
````
./client --host=10.10.1.1 --port=3000 -f /tmp/fs_to_mount
````

### Troubleshooting
If you face any issues running the client, check the following:
- The mount path is not currently being accessed.
- The path is not already mounted (Check using `findmnt`, Unmount using `sudo umount /path-to-mount`)
- The server is not running as `localhost` in a distributed scenario. Ensure that the server gets its own IP as an argument in this case.

### Tests
The tests are included in the tests/ directory. They run different workloads on the machine and collect benchmarking statistics.
Example usage:
````
gcc tester.c -o tester ; ./tester
rm -rf ~/.fuse_server/* ; g++ bechmark.cpp -std=c++17 -O3 -pthread &&   ./a.out
````
### Sample Workloads
The scripts directory includes more sample workloads that can be run as below. The mount path is hardcoded in the scripts and must be edited.
````
./run_multi_client.sh <no of clients> '../build/bin/client --host=<server ip> --port=<server port>' '<your-install-path>/739-p2c/scripts/workload.sh'
````
