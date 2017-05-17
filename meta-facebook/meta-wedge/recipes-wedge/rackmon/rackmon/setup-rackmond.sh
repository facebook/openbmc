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

runsv /etc/sv/rackmond > /dev/null 2>&1 &
runsv /etc/sv/rackmon-gpio > /dev/null 2>&1 &
sv $1 rackmond
sv $1 rackmon-gpio
