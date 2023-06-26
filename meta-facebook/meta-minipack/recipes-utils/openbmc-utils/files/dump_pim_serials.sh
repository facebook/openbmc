#!/bin/bash
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

for index in 1 2 3 4 5 6 7 8
do
    pim_path=${SMBCPLD_SYSFS_DIR}/pim_fpga_cpld_${index}_prsnt_n_status
    pim_prsnt=$(cat $pim_path 2> /dev/null | head -n 1)
    cache_file="/tmp/pim${index}_serial.txt"
    
    # Refresh pimserial cache file when
    #   1) the pim is present, and
    #   2) the cache file doesn't exist or it doesn't contain valid strings
    if [ ! -s "$cache_file" ] || [ -z "$(cat $cache_file)" ]; then
        if [ "${pim_prsnt}" == 0x0 ]; then
            /usr/local/bin/peutil $index |grep Product|grep Serial|cut -d ' ' -f 4 > "$cache_file"
        fi
    fi
    serial="$(cat $cache_file)"
    echo PIM${index} : ${serial}
done

