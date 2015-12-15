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
# Provides:          setup-rest-api
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set REST API handler
### END INIT INFO

# source function library
. /etc/init.d/functions

ACTION="$1"
CMD="/usr/local/bin/rest.py"
case "$ACTION" in
  start)
    echo -n "Setting up REST API handler: "
    pid=$(ps | grep -v grep | grep $CMD | awk '{print $1}')
    if [ $pid ]; then
      echo "already running"
    else
      $CMD > /tmp/rest_start.log 2>&1 &
      echo "done."
    fi
    ;;
  stop)
    echo -n "Stopping REST API handler: "
    pid=$(ps | grep -v grep | grep $CMD | awk '{print $1}')
    if [ $pid ]; then
      kill $pid
    fi
    echo "done."
    ;;
  restart)
    echo -n "Restarting REST API handler: "
    pid=$(ps | grep -v grep | grep $CMD | awk '{print $1}')
    if [ $pid ]; then
      kill $pid
    fi
    sleep 1
    $CMD > /tmp/rest_start.log 2>&1 &
    echo "done."
    ;;
  status)
    if [[ -n $(ps | grep -v grep | grep $CMD | awk '{print $1}') ]]; then
      echo "REST API handler is running"
    else
      echo "REST API is stopped"
    fi
    ;;
  *)
    N=${0##*/}
    N=${N#[SK]??}
    echo "Usage: $N {start|stop|status|restart}" >&2
    exit 1
    ;;
esac

exit 0
