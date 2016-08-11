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

if [ $# -ne 2 ]; then
    exit -1
fi

declare -A fcb_gpio_map
fcb_gpio_map[fcb1]=456
fcb_gpio_map[fcb2]=464
fcb_gpio_map[fcb3]=472
fcb_gpio_map[fcb4]=480

card="${1,,}"
img="$2"

base=${fcb_gpio_map[$card]}

if [ -z "$base" ]; then
    echo "${1} is not a correct FCB card name."
    exit -1
fi

ispvm dll /usr/lib/libcpldupdate_dll_gpio.so "${img}" --tms ${base} --tck $((base+1)) --tdi $((base+2)) --tdo $((base+3))
