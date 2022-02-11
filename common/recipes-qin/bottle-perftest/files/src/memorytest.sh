#!/bin/bash

iteration=4100
for ((i=1;i<iteration;i=i*2));
do
  python service-memorytest.py --i $i &
  spid=$(echo $!)
  python ipc-memorytest.py --i $i &
  ipid=$(echo $!)

  # wait for the connection to service to be set up
  python caller-memorytest.py

  echo $i routes: $(top -b -n 1 | grep Mem)
  kill $spid $ipid
done
