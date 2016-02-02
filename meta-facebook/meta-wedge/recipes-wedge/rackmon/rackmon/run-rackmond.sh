#!/bin/sh
(sleep 5; PYTHONPATH=/etc python /etc/rackmon-config.py) &
export RACKMOND_FOREGROUND=1
exec /usr/local/bin/rackmond

