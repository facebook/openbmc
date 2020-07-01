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
# Provides:          oob-nic
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:      0 6
# Short-Description: One of the first scripts to be executed. Starts or stops
#               the OOB NIC.
#
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/oob-nic
NAME=oob-nic
DESC="OOB NIC Driver"

# source function library
. /etc/init.d/functions

test -f $DAEMON || exit 0

# enable the isolation buffer
. /usr/local/bin/openbmc-utils.sh

fix_etc_interfaces() {
    local intf_conf board_rev board_type enable_oob
    if wedge_should_enable_oob; then
        intf_conf="/etc/network/interfaces"
        if ! grep oob $intf_conf > /dev/null 2>&1; then
            echo >> $intf_conf
            echo "auto oob" >> $intf_conf
            echo "iface oob inet dhcp" >> $intf_conf
        fi
    fi
}

STOPPER=
ACTION="$1"

case "$ACTION" in
  start)
    echo -n "Starting $DESC: "
    if [ ! -d /dev/net ]
    then
      mkdir /dev/net
    fi
    if [ ! -f /dev/net/tun ]
    then
      mknod /dev/net/tun c 10 200
      chmod 666 /dev/net/tun
    fi
    fix_etc_interfaces
    $DAEMON > /dev/null 2>&1 &
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
    fix_etc_interfaces
    $DAEMON > /dev/null 2>&1 &
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
