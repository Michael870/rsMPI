#!/usr/bin/python
import sys

comp_time = []
total_time = []

with open(sys.argv[1], 'r') as f:
    for line in f:
        if "Computation time" in line:
            comp_time.append(line.split()[3])
        elif "Total time" in line:
            total_time.append(line.split()[3])

print("Computation time")
for item in comp_time:
    print(item)
#print "Total time"
#for item in total_time:
#    print item
