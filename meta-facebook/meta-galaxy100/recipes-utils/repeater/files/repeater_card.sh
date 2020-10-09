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

. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

I2C_BUS=0
RETIMER_ADDR=0x58

usage() {
	echo "Usage: $0 start"
}

do_start() {
	#i2cset -f -y ${I2C_BUS} ${RETIMER_ADDR} 0 0
	exit 0
}

if [ $# -lt 1 ]; then
	usage
	exit -1
fi

case "$1" in
	start)
		do_start
		;;
	*)
		usage
		exit -1
		;;
esac

exit $?
