#!/bin/bash

echo "mkdir: "
time mkdir -p {a,b,c,d,e,f}/{g,h,i,j,k}/{a,b,c,d,e,f}/{g,h,i,j,k}/{a,b,c,d,e,f}/{g,h,i}/


echo "scandir: "
time find . -type d | wc -l


echo "rmdir: "
time rm -rf {a,b,c,d,e,f}/


echo "create 1000 files of 4kb"
function c1000_4() {
    mkdir 1000_of_4kb
    for i in `seq 1 1000` do;
        head -c 4K /dev/urandom > 1000_of_4kb/$i
    done
}
time c1000_4

echo "read 1000 files of 4kb"
time cat 1000_of_4kb/* > /dev/null

rm -rf 1000_of_4kb
