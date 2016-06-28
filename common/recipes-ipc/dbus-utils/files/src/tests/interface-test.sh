#!/bin/bash
eval `dbus-launch --auto-syntax`
dbuspid=$(ps | grep dbus | grep fork | grep -o "^ *[0-9]*")
./dbus-interface-test &
serverpid=$(ps | grep dbus-interface-test | grep -o "^ *[0-9]*")

sleep 1

dbus-send --session --type=method_call --print-reply \
          --dest=org.openbmc.TestServer \
            /org/openbmc/test \
            org.openbmc.TestInterface.Ping

kill $serverpid
kill $dbuspid
