#!/bin/sh

#for n in 2 4 6 8 16 32 64 128 256 512 1024 2048 4096 
for n in 0 1 2 3 4 5 6 7 8 9 10 12 14 16
do
 ./waf --run "scratch/HyScale --k=$n" >> statistics/hyscale.log 2>&1
done
