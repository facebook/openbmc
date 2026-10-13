#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

FAN_PREFIX="/sys/bus/i2c/devices/6-0060/fan"

echo -e "\n################################"
echo "######## FAN DEBUG INFO ########"
echo "################################"

for i in 1 2 3 4 5; do
	echo -e "\n##### FAN $i DEBUG INFO #####"
	for attr in pwm present; do
		f="${FAN_PREFIX}${i}_${attr}"
		if [ ! -f "$f" ]; then
			echo "$f doesn't exist!"
		else
			echo "$attr: $(head -n 1 "$f")"
		fi
	done
done

echo -e "\n##### FAN SPEED LOGS #####"
if [ ! -x /usr/local/bin/get_fan_speed.sh ]; then
	echo "/usr/local/bin/get_fan_speed.sh doesn't exist!"
else
	# Sampled twice, as show_tech.py did, to show whether fans are moving.
	/usr/local/bin/get_fan_speed.sh
	sleep 0.5
	/usr/local/bin/get_fan_speed.sh
fi

echo -e "\n##### FAN CPLD I2CDUMP #####"
if [ ! -e /dev/i2c-6 ]; then
	echo "/dev/i2c-6 doesn't exist!"
else
	i2cdump -f -y 6 0x60
fi
