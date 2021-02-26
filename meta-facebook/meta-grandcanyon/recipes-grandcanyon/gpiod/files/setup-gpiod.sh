#!/bin/sh
#
# Copyright 2021-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-gpiod
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Start gpiod daemon
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup gpiod for fbgc..."
runsv /etc/sv/gpiod > /dev/null 2>&1 &
echo "done."
