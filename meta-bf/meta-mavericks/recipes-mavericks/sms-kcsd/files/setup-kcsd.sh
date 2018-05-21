#! /bin/sh
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
# Provides:          kcsd
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:      0 6
# Short-Description: Provides kcs message tx/rx service
#
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
DAEMON=/usr/local/bin/kcsd
NAME=kcsd
DESC="KCS Rx/Tx Daemon"

test -f $DAEMON || exit 0

STOPPER=
ACTION="$1"

case "$ACTION" in
  start)
    echo -n "Starting $DESC: "
    $DAEMON 2 127 > /dev/null 2>&1 &
    $DAEMON 1 127 > /dev/null 2>&1 &
    echo "$NAME."
    ;;
  stop)
    echo -n "Stopping $DESC: "
    start-stop-daemon --stop --quiet --exec $DAEMON
    echo "$NAME."
    ;;
  restart|force-reload)
    echo -n "Restarting $DESC: "
    start-stop-daemon --stop --quiet --exec $DAEMON
    sleep 1
    $DAEMON 2 127 > /dev/null 2>&1 &
    $DAEMON 1 127 > /dev/null 2>&1 &
    echo "$NAME."
    ;;
  status)
    status $DAEMON
    exit $?
    ;;
  *)
    N=${0##*/}
    N=${N#[SK]??}
    echo "Usage: $N {start|stop|status|restart|force-reload}" >&2
    exit 1
    ;;
esac

exit 0
