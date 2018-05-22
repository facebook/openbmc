#!/bin/sh
(sleep 5; PYTHONPATH=/etc python /etc/rackmon-config.py) &
export RACKMOND_FOREGROUND=1
# Give PSUs 2ms of grace time to let go of the bus after they respond to a
# command
export RACKMOND_MIN_DELAY=2000
exec /usr/local/bin/rackmond

