#!/bin/sh
#
# Copyright 2017-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-ecc-mond
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Start ECC monitor daemon
### END INIT INFO

echo -n "Setup ECC Monitor Daemon.."
  runsv /etc/sv/ecc-mond > /dev/null 2>&1 &
echo "done."
