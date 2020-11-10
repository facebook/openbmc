#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

usage() {
	program=$(basename "$0")
	echo "Usage:"
	echo "  $program <led>"
	echo "  <led>: sys/fan/psu/scm"
	echo "  $program without option to display all leds"
	echo
	echo "Output:"
	echo "  <led>: color"
	exit 1
}

read_led_color_value() {
	case $1 in
	"sys" | "fan" | "psu" | "scm")
		cat "/sys/class/leds/$1/multi_intensity"
	;;
	*)
		usage
	;;
	esac
}

# color: red-blue-green (value: 0-255)
#   red(255 0 0)
#   blue(0 255 0)
#   green(0 0 255)
#   amber(255 0 45)
print_led_color() {
	color=""
	case $(read_led_color_value "$1") in
	"255 0 0")
		color="red"
		;;
	"0 255 0")
		color="blue"
		;;
	"0 0 255")
		color="green"
		;;
	"255 0 45")
		color="amber"
		;;
	"0 0 0")
		color="off"
		;;
	*)
		color="unknown"
		;;
	esac
	echo "$1: $color"
}

if [ $# -ge 1 ]; then
	led=$1
	case $1 in
		"?" | "help")
			usage
			;;
		"sys" | "fan" | "psu" | "scm")
			print_led_color "$led"
			exit 0
			;;
		*)
			usage
			;;
	esac
fi

leds=("sys" "fan" "psu" "scm")
for led in ${leds[*]}
do
	print_led_color "$led"
done
