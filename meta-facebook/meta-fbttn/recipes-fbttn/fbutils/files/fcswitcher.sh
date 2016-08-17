#! /bin/sh
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
# Provides:          usbcons
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:      0 6
# Short-Description: Creates a virtual USB serial device and starts a console
#                    on it.
#
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin
NAME="FC Switcher"
DESC="FC Failover Daemon"

# source function library
. /etc/init.d/functions

. /usr/local/fbpackages/utils/ast-functions

STOPPER=
ACTION="$1"

case "$ACTION" in
  start)
    if [ "$(wedge_board_type)" = "LC" ]; then
      # Ability to prevent this from starting by editing cmdline in u-boot.
      # Keeping this here until I get gadget switching working properly. (t4906522)
      /usr/local/bin/watch-fc.sh > /dev/null 2>&1 &
      echo "$NAME."
    else
      echo 'skipping watch-fc.sh: only necessary on six-pack line cards.'
    fi
    ;;
  stop)
    echo -n "Stopping $DESC: "
    killall watch-fc.sh
    echo "$NAME."
    ;;
  restart|force-reload)
    echo -n "Restarting $DESC: "
    killall watch-fc.sh
    if [ "$(wedge_board_type)" = "LC" ]; then
      sleep 1
      /usr/local/bin/watch-fc.sh > /dev/null 2>&1 &
      echo "$NAME."
    else
      echo 'skipping watch-fc.sh: only necessary on six-pack line cards.'
    fi
    ;;
  status)
    status watch-fc.sh
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
