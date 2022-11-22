#!/bin/bash
# This is workaround for NC-SI eth0 not bring up after AC-Cycle.

if ! grep -q eth0 /var/run/ifstate; then
    logger "bring up eth0 again"
    /sbin/ifup eth0 &
else
    logger "eth0 bring up already"
fi
