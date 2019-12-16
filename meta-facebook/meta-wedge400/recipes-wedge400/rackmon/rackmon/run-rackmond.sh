#!/bin/sh
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
ret=0

PIDFILE="/var/run/run-rackmond.pid"
check_duplicate_process()
{
    exec 2>$PIDFILE
    flock -n 2 || (echo "Another process is running" && exit 1)
    ret=$?
    if [ $ret -eq 1 ]; then
        exit 1
    fi
    pid=$$
    echo $pid 1>&200
}
check_duplicate_process
(sleep 5; PYTHONPATH=/etc python /etc/rackmon-config.py) &
export RACKMOND_FOREGROUND=1
# Give PSUs 2ms of grace time to let go of the bus after they respond to a
# command
export RACKMOND_MIN_DELAY=2000
exec /usr/local/bin/rackmond

