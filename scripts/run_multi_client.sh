#!/bin/bash
num=$1
crun_arg="$2"
wl="$3"
set -x
cpids=()
for i in `seq 1 $num`; do
   mount_pt=/tmp/fs_$i
   sudo umount -f $mount_pt
   rm -rf $mount_pt
    mkdir -p $mount_pt
    $crun_arg -f $mount_pt &> ${mount_pt}_cl  &
    pid=$!
    cpids+=($pid)
done

sleep 3

for i in `seq 1 $num`; do
   mount_pt=/tmp/fs_$i
   cd $mount_pt
   
   $wl &> ${mount_pt}_wl_${num} &
done

wait
echo killing all clients now >&2

for i in ${cpids[@]}; do
	kill -9 $i
done

for i in `seq 1 $num`; do
   mount_pt=/tmp/fs_$i
   sudo umount -f $mount_pt
done
