#!/bin/bash
#This is to run multiple times of "mpirun" for 
#each cluster size and each failure distribution
#to calculate average completion time (time.txt) 
#and number of failures (failure.txt)
#by Michael Cui
#10/2/2014

run=5
total_time=0
timefile="time.txt"
#s_size=`expr $s_end - $s_start + 1`


#if [ $# -ne 3 ]
#then 
#	echo "Wrong useage! CORRECT USAGE: ./avg.sh w N MTBF"
#	exit 1
#fi

if [ -f $timefile ]
then
	rm $timefile
fi

#if [ -f $failfile ]
#then
#	rm $failfile
#fi
for (( step = 0; step < $run; step++ ))
do
    mpirun -np $1 -rf $2 $3 >> $timefile
#    single_time=`/usr/bin/time -f "%e" mpirun -np $1 -rf $2 $3 2>&1 1>/dev/null`
#    echo $single_time >> $timefile
#    total_time=`echo $total_time + $single_time | bc -l`
done
#echo $total_time / $run | bc -l >> $timefile

#for (( fd = 0; fd <=2; fd++ ))
#do
#	if [ $fd -eq 0 ]
#	then
#		echo "exponential distribution (s from $s_start to $s_end)" >> $timefile
#		echo "exponential distribution (s from $s_start to $s_end)" >> $failfile
#	elif [ $fd -eq 1 ] 
#	then 
#		echo "uniform distribution (s from $s_start to $s_end)" >> $timefile
#		echo "uniform distribution (s from $s_start to $s_end)" >> $failfile
#	else
#		echo "weibull distribution (s from $s_start to $s_end)" >> $timefile
#		echo "weibull distribution (s from $s_start to $s_end)" >> $failfile
#	fi
#	for (( s = $s_start; s <= $s_end; s = $s + $s ))
#	do
#		#echo $s
#		ctime=0
#		cfailure=0
#		for (( i = 0; i < $run; i++ ))
#		do
#			./sc $1 $2 $3 $s $fd >> intermediate.txt
#			j=0
#			while read line
#			do
#				j=`expr $j + 1`
#				if [ $j -eq 1 ]
#				then
#					ctime="$ctime + $line"
#				elif [ $j -eq 2 ]
#				then
#					cfailure="$cfailure + $line"
#				fi
#			done < intermediate.txt
#			rm intermediate.txt
#		done
#		ctime="($ctime) / $run"
#		echo $ctime | bc -l >> $timefile
#		cfailure="($cfailure) / $run"
#		echo $cfailure | bc -l >> $failfile
#	done
#done





