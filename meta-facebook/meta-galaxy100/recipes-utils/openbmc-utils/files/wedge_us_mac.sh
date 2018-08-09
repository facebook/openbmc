#!/bin/bash
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh

if ! wedge_is_us_on; then
    echo "Cannot retrive microserver MAC when microserver is powered off." 1>&2
    exit -2
fi

count=0
while [ $count -lt 10 ]
do
  mac=$(cat /sys/class/i2c-adapter/i2c-0/0-0033/mac_addr)

	if [ -n "$mac" -a "${mac/X/}" = "${mac}" ]; then
	    echo $mac
	    exit 0
	fi
	count=$(($count + 1))
	sleep 1
done

if [ $count -ge 10 ]; then
	echo "Cannot find out the microserver MAC" 1>&2
	exit -1
fi
