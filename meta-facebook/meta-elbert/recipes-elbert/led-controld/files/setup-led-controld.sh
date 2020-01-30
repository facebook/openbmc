#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-front-paneld
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Start front panel control daemon
### END INIT INFO

echo "Setup Front Panel Daemon.."
runsv /etc/sv/led-controld > /dev/null 2>&1 &
echo "done."
