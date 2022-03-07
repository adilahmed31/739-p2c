#!/bin/bash

rv=$(cat /proc/sys/kernel/random/uuid | sed 's/[-]//g' | head -c 20)
echo "create 2000 files of 4kb x 4"
function c1000_4() {
    mkdir 1000_of_4kb_${rv}
    for i in `seq 1 2000`; do
        head -c 4K /dev/urandom >> 1000_of_4kb_${rv}/$i
        head -c 4K /dev/urandom >> 1000_of_4kb_${rv}/$i
        head -c 4K /dev/urandom >> 1000_of_4kb_${rv}/$i
        head -c 4K /dev/urandom >> 1000_of_4kb_${rv}/$i
    done
}
time -p c1000_4

echo "read 2000 files of 16kb"
time -p cat 1000_of_4kb_${rv}/* > /dev/null

rm -rf 1000_of_4kb_${rv}

echo "mkdir: "
time -p mkdir -p ${rv}/{a,b,c,d,e,f}/{g,h,i,j,k}/{a,b,c,d,e,f}/{g,h,i,j,k}/{a,b,c,d,e,f}/{g,h,i}/


echo "scandir: "
time -p find ${rv} -type d | wc -l


echo "rmdir: "
time -p rm -rf ${rv}


