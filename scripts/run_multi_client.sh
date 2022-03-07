#!/bin/bash
num=$1
crun_arg="$2"
wl="$3"
for i in `seq 1 $num`; do
   mount_pt=/tmp/fs_$i
    mkdir -p $mount_pt
    $crun_arg -f $mount_pt &> ${mount_pt}_cl  &
done

sleep 3

for i in `seq 1 $num`; do
   mount_pt=/tmp/fs_$i
   cd $mount_pt
   $wl &> ${mount_pt}_wl &
done
