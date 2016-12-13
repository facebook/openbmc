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



lc_r=0
((val=$(i2cget -f -y 12 0x31 0x3 2> /dev/null | head -n 1)))
((right=$(i2cget -f -y 12 0x31 0x5 2> /dev/null | head -n 1)))
if [ $val -lt 8  -a $right -eq 1 ]; then
	lc_r=1
fi
lc_r_rec_msg() {
	if [ $lc_r -eq 1 ]; then
		i2cset -f -y 12 0x31 0x28 0x0 2> /dev/null
		tmp=$(i2cget -f -y 12 0x31 0x28 2> /dev/null)
		val=$(($tmp & 0x70))
		tmp2=$(i2cget -f -y 12 0x31 0x28 2> /dev/null) #double check
		if [ $val -eq $((0x50)) -a $tmp = $tmp2 ]; then
			echo "Receive msg to power on COMe because of SCM CPLD upgrading"
			i2cset -f -y 12 0x31 0x28 0x05 2> /dev/null
			#power off COMe
			#i2cset -f -y 0 0x3e 0x10 0xfd 2> /dev/null
			#sleep 2
			#i2cset -f -y 0 0x3e 0x10 0xfe 2> /dev/null

			#reset TH
			#i2cset -f -y 12 0x31 0x12 0xf2 2> /dev/null
			#sleep 1
			#i2cset -f -y 12 0x31 0x12 0xf3 2> /dev/null
			
			i2cset -f -y 12 0x31 0x10 0xfd			

			power_status=1
			count=0
			delay_flag=0
		fi
	fi
}

power_status=0
count=0
delay_flag=0
while true; do
	lc_r_rec_msg
	if [ $power_status -eq 1 -a $delay_flag -eq 1 ]; then
		i2cset -f -y 12 0x31 0x10 0xff
		restore_us_com
		power_status=2
	elif [ $power_status -eq 2 -a $delay_flag -eq 2 ]; then
		((temp=$(i2cget -f -y 0 0x3e 0x12 2> /dev/null | head -n 1)))
		if [ $temp != 15 ] ; then
			echo "Power on COM-e fail, recovery..."
			come_recovery
		else
			echo "Power on COM-e OK"
			power_status=0
			delay_flag=0
		fi
	fi
	if [ $power_status -ne 0 ]; then
		if [ $count -eq 30 ]; then
			delay_flag=1
		elif [ $count -eq 70 ]; then
			delay_flag=2
		elif [ $count -ge 430 ]; then
			delay_flag=2
			count=71
		else
			delay_flag=0
		fi
		count=$(($count+1))
	fi
	usleep 500000
done
