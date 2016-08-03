#!/bin/bash
eval `dbus-launch --auto-syntax`
dbuspid=$(ps | grep dbus | grep fork | grep -o "^ *[0-9]*")
./dbus-testserver &
serverpid=$(ps | grep dbus-testserver | grep -o "^ *[0-9]*")

iteration=1000
for ((i=0;i<iteration;i++));
do
  dbus-send --session --type=method_call \
            --dest=org.openbmc.TestServer \
            /org/openbmc/test \
            org.openbmc.TestInterface.HelloWorld \
            string:'greeting'
done
kill $serverpid
kill $dbuspid
