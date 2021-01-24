#!/bin/sh
#
# Copyright 2021-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-gpiointrd
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Start gpiointrd daemon
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup gpiointrd for fbgc..."
runsv /etc/sv/gpiointrd > /dev/null 2>&1 &
echo "done."
