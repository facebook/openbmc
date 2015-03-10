#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-fan
# Required-Start:    board-id
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set fan speed
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

# Enable the isolation buffer
wedge_iso_buf_enable

echo -n "Setup fan speed... "
/usr/local/bin/init_pwm.sh
/usr/local/bin/set_fan_speed.sh 50
/usr/local/bin/fand
echo "done."
