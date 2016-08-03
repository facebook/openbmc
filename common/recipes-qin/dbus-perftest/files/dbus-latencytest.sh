#!/bin/bash
eval `dbus-launch --auto-syntax`
dbuspid=$(ps | grep dbus | grep fork | grep -o "^ *[0-9]*")
./dbus-testserver &
serverpid=$(ps | grep dbus-testserver | grep -o "^ *[0-9]*")

./dbus-latencytest

kill $serverpid
kill $dbuspid
