#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
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

echo -n "Setup gpiointrd for fby3..."
runsv /etc/sv/gpiointrd > /dev/null 2>&1 &

bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  #The BMC of class2
  #gpiointrd only supports the hot service and the hot service is not supported on class 2.
  #stop it
  sv stop gpiointrd
fi

echo "done."


