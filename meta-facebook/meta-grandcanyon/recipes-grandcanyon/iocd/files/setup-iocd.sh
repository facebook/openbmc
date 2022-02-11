#!/bin/sh
#
# Copyright 2021-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-iocd
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Start IOC daemon
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup iocd for fbgc..."

get_chassis_type
uic_type=$?

runsv /etc/sv/iocd_11 > /dev/null 2>&1 &

if [ "$uic_type" = "$UIC_TYPE_7_HEADNODE" ]; then
  runsv /etc/sv/iocd_12 > /dev/null 2>&1 &
fi

echo "done."

