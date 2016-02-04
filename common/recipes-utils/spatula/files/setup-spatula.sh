#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
# Provides:          setup-spatula-wrapper
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set Spatula Wrapper
### END INIT INFO

# source function library
. /etc/init.d/functions

. /etc/default/spatula.conf

ACTION="$1"
CMD="/usr/local/bin/spatula_wrapper.py ${SPATULA_ARGS}"
case "$ACTION" in
  start)
    echo -n "Setting up Spatula: "
    pid=$(ps | grep -v grep | grep "$CMD" | awk '{print $1}')
    if [ $pid ]; then
      echo "already running"
    else
      $CMD > /var/log/spatula.log 2>&1 &
      echo "done."
    fi
    ;;
  stop)
    echo -n "Stopping Spatula: "
    pid=$(ps | grep -v grep | grep "$CMD" | awk '{print $1}')
    if [ $pid ]; then
      kill $pid
    fi
    echo "done."
    ;;
  restart)
    echo -n "Restarting Spatula: "
    pid=$(ps | grep -v grep | grep "$CMD" | awk '{print $1}')
    if [ $pid ]; then
      kill $pid
    fi
    sleep 1
    $CMD > /var/log/spatula.log 2>&1 &
    echo "done."
    ;;
  status)
    if [[ -n $(ps | grep -v grep | grep "$CMD" | awk '{print $1}') ]]; then
      echo "Spatula is running"
    else
      echo "Spatula is stopped"
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

