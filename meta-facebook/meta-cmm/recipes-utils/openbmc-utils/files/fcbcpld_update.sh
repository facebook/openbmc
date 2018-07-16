#!/bin/bash
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

source /usr/local/bin/gpio-utils.sh

if [ $# -ne 2 ]; then
    exit -1
fi

declare -A fcb_gpio_map
fcb1_gpiochip=$(gpiochip_lookup_by_i2c_path "172-0022")
fcb2_gpiochip=$(gpiochip_lookup_by_i2c_path "180-0022")
fcb3_gpiochip=$(gpiochip_lookup_by_i2c_path "188-0022")
fcb4_gpiochip=$(gpiochip_lookup_by_i2c_path "196-0022")
fcb_gpio_map[fcb1]=$(gpiochip_get_base ${fcb1_gpiochip})
fcb_gpio_map[fcb2]=$(gpiochip_get_base ${fcb2_gpiochip})
fcb_gpio_map[fcb3]=$(gpiochip_get_base ${fcb3_gpiochip})
fcb_gpio_map[fcb4]=$(gpiochip_get_base ${fcb4_gpiochip})

card="${1,,}"
img="$2"

base=${fcb_gpio_map[$card]}

if [ -z "$base" ]; then
    echo "${1} is not a correct FCB card name."
    exit -1
fi

ispvm dll /usr/lib/libcpldupdate_dll_gpio.so "${img}" --tms ${base} --tck $((base+1)) --tdi $((base+2)) --tdo $((base+3))
