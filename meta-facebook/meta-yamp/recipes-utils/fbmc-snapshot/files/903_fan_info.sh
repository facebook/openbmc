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

FAN_CPLD="/sys/bus/i2c/devices/13-0060"

echo -e "\n################################"
echo "######## FAN DEBUG INFO ########"
echo "################################"

echo -e "\n##### FAN CARD OVERTEMP #####"
if [ ! -f "$FAN_CPLD/fan_card_overtemp" ]; then
	echo "$FAN_CPLD/fan_card_overtemp doesn't exist!"
else
	cat "$FAN_CPLD/fan_card_overtemp"
fi

for i in 1 2 3 4 5; do
	echo -e "\n##### FAN $i DEBUG INFO #####"
	for attr in pwm present id present_change; do
		f="${FAN_CPLD}/fan${i}_${attr}"
		if [ ! -f "$f" ]; then
			echo "$f doesn't exist!"
		else
			echo "$attr: $(head -n 1 "$f")"
		fi
	done

	echo "### FAN $i LED INFO ### 0x0 - ON, 0x1 - OFF"
	for attr in led_red led_green; do
		f="${FAN_CPLD}/fan${i}_${attr}"
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
if [ ! -e /dev/i2c-13 ]; then
	echo "/dev/i2c-13 doesn't exist!"
else
	i2cdump -f -y 13 0x60
fi
