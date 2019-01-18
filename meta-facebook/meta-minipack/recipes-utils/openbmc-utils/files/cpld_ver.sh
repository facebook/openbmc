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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

dump_cpld_version() {
    local cpld_dir=$1
    local cpld_name=$2

    cpld_ver=`head -n 1 ${cpld_dir}/cpld_ver 2> /dev/null`
    if [ $? -ne 0 ]; then
        echo "${cpld_name} is not detected"
        return
    fi
    cpld_sub_ver=`head -n 1 ${cpld_dir}/cpld_sub_ver 2> /dev/null`
    if [ $? -ne 0 ]; then
        echo "${cpld_name} is not detected"
        return
    fi

    echo "${cpld_name}: $(($cpld_ver)).$(($cpld_sub_ver))"
}

dump_cpld_version ${SMBCPLD_SYSFS_DIR} "SMBCPLD"
usleep 50000

dump_cpld_version ${SCMCPLD_SYSFS_DIR} "SCMCPLD"
usleep 50000

dump_cpld_version ${TOP_FCMCPLD_SYSFS_DIR} "Top_FCMCPLD"
usleep 50000

dump_cpld_version ${BOTTOM_FCMCPLD_SYSFS_DIR} "Bottom_FCMCPLD"
usleep 50000

board_rev=$(wedge_board_rev)
if [ $board_rev -ne 4 ]; then
    dump_cpld_version ${LEFT_PDBCPLD_SYSFS_DIR} "Left_PDBCPLD"
    usleep 50000

    dump_cpld_version ${RIGHT_PDBCPLD_SYSFS_DIR} "Right_PDBCPLD"
fi
