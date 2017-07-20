#!/bin/bash

runs=5

for (( len = 0; len < 250; len = $len + 50 ))
do
    timefile="msg_$len.txt"
    rtimefile="rmsg_$len.txt"
    for (( iter = 0; iter < $runs; iter++ ))
    do
        mpirun --oversubscribe -np 16 --rf /pylon1/ci4s84p/xiaolong/rankfiles/rankfile_16_8 loops $len >> $timefile 
        #mpirun --oversubscribe -np 40 --rf /pylon1/ci4s84p/xiaolong/rankfiles/rankfile_16_8 rloops $len >> $rtimefile 
    done
done
