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

# Convert card string to upper case
card="${1^^}"
if [ ${card} != "FCB1" && ${card} != "FCB2" && \
     ${card} != "FCB3" && ${card} != "FCB4" ]; then
    echo "${1} is not a correct FCB card name."
    exit -1
fi
img="$2"

ispvm dll /usr/lib/libcpldupdate_dll_gpio.so "${img}" \
    --tms ${card}_CPLD_TMS \
    --tck ${card}_CPLD_TCK \
    --tdi ${card}_CPLD_TDI \
    --tdo ${card}_CPLD_TDO
