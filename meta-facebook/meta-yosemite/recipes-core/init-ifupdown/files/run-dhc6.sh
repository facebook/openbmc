#!/bin/sh
logger "run-dhc6.sh: dhclient -6 Started.."
pid="/var/run/dhclient6.eth0.pid"
exec dhclient -6 -d -pf ${pid} eth0 "$@"
