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

. /usr/local/fbpackages/utils/ast-functions

function expansion_init() {
  echo ""
}

function server_init() {
  echo -n "Setup Front Panel Daemon.."
  runsv /etc/sv/front-paneld > /dev/null 2>&1 &
  echo "done."
}

Location=$(gpio_get BMC_LOCATION)
#Location: 1 exp; 0 baseboard
if [ $Location == "1" ]; then
  expansion_init
else
  server_init
fi
