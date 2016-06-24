#!/bin/bash

eval `dbus-launch --auto-syntax`
iteration=4100
for ((i=1;i<iteration;i=i*2));
do
  dbuspid=$(ps | grep dbus | grep fork | grep -o "^ *[0-9]*")
  ./dbus-mem-testserver $i &
  serverpid=$(echo $!)

  sleep 1
  echo $i objects: $(top -b -n 1 | grep Mem)
  kill $serverpid
done
kill $dbuspid
