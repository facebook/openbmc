#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

### BEGIN INIT INFO
# Provides:          bmc-log
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Collect micro-server kernel logs through serial port
### END INIT INFO

. /etc/default/bmc-log
. /etc/init.d/functions

DAEMON=/usr/sbin/bmc-log
NAME=bmc-log
DESC="Micro-Server log collection"

TTY=${US_TTY:-/dev/ttyS1}
IP=${LOG_SERVER_IP_VERSION:-4}
LOG_SERVER=${LOG_SERVER_NAME:-}
PORT=${LOG_SERVER_PORT:-}
BAUD_RATE=${TTY_BAUD_RATE:-}

if [ -z "$LOG_SERVER" ] || [ -z "$PORT" ]
then
	echo "Error: Server and/or port not set"
	exit 0
fi


ACTION="$1"

case "$ACTION" in
  start)
  	echo -e "Starting $DESC"
	$DAEMON $TTY $IP $LOG_SERVER $PORT $BAUD_RATE
    ;;
  stop)
    echo -e "Stopping $DESC: "
    start-stop-daemon --stop --quiet --exec $DAEMON
    ;;
   restart|force-reload)
    echo -e "Restarting $DESC: "
    start-stop-daemon --stop --quiet --exec $DAEMON
    sleep 1
    $DAEMON $TTY $IP $LOG_SERVER $PORT $BAUD_RATE
    ;;
  status)
    stat $DAEMON
    exit $?
    ;;
  *)
    N=${0##*/}
    N=${N#[SK]??}
    echo "Usage: $N {start|stop|status|restart|force-reload}" >&2
    exit 1
    ;;
esac
