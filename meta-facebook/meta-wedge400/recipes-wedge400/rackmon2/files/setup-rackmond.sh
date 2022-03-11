#!/bin/bash
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-rackmond
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Start Rackmon service
### END INIT INFO

#
# No need to start rackmond if the according tty device cannot be found:
#
BMC_PSU_TTY="/dev/ttyUSB0"
if ! [ -e "$BMC_PSU_TTY" ]; then
    echo "rackmond not started: $BMC_PSU_TTY does not exist!"
    exit 1
fi

runsv /etc/sv/rackmond > /dev/null 2>&1 &
sv "$1" rackmond
