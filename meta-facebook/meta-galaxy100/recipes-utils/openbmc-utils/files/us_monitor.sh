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

. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# Because the voltage leak from uS COM pins could cause uS to struck when
# transitting from S5 to S0, we will need to explicitely pull down uS COM
# pins before powering off/reset and restoring COM pins after

restore_us_com() {
	repeater_config
	wedge_power_on_board
	i2cset -f -y 0 0x3e 0x10 0xff 2> /dev/null
}

val=0
ret=0
while true; do
	galaxy100_scm_is_present
	ret=$?
	if [ $ret -eq 0 ] && [ $val -eq 1 ]; then
		echo "SCM is inserted, Power ON!"
		sleep 15
		restore_us_com
	fi
	val=$ret
	usleep 500000
done
