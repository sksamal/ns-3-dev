#!/bin/sh

#for n in 2 4 6 8 16 32 64 128 256 512 1024 2048 4096 
for n in 2 4 6 8 10 12 14 16 18 20
do
 ./waf --run "scratch/BCube --n=$n" >> statistics/fat-tree.log 2>&1
done
