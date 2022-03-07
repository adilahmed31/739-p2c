#!/bin/bash


echo "create 2000 files of 4kb x 4"
function c1000_4() {
    mkdir 1000_of_4kb
    for i in `seq 1 2000`; do
        head -c 4K /dev/urandom >> 1000_of_4kb/$i
        head -c 4K /dev/urandom >> 1000_of_4kb/$i
        head -c 4K /dev/urandom >> 1000_of_4kb/$i
        head -c 4K /dev/urandom >> 1000_of_4kb/$i
    done
}
time -p c1000_4

echo "read 2000 files of 16kb"
time -p cat 1000_of_4kb/* > /dev/null

rm -rf 1000_of_4kb

echo "mkdir: "
time -p mkdir -p {a,b,c,d,e,f}/{g,h,i,j,k}/{a,b,c,d,e,f}/{g,h,i,j,k}/{a,b,c,d,e,f}/{g,h,i}/


echo "scandir: "
time -p find . -type d | wc -l


echo "rmdir: "
time -p rm -rf {a,b,c,d,e,f}/


