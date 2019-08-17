#!/bin/bash
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

board_rev=$(wedge_board_rev)

smb_ver=$(cat $SMBCPLD_SYSFS_DIR/cpld_ver | head -n 1)
smb_sub_ver=$(cat $SMBCPLD_SYSFS_DIR/cpld_sub_ver | head -n 1)
pwr_ver=$(cat $PWRCPLD_SYSFS_DIR/cpld_ver | head -n 1)
pwr_sub_ver=$(cat $PWRCPLD_SYSFS_DIR/cpld_sub_ver | head -n 1)
scm_ver=$(cat $SCMCPLD_SYSFS_DIR/cpld_ver | head -n 1)
scm_sub_ver=$(cat $SCMCPLD_SYSFS_DIR/cpld_sub_ver | head -n 1)
fcm_ver=$(cat $FCMCPLD_SYSFS_DIR/cpld_ver | head -n 1)
fcm_sub_ver=$(cat $FCMCPLD_SYSFS_DIR/cpld_sub_ver | head -n 1)

echo "SMB_SYSCPLD: $(($smb_ver)).$(($smb_sub_ver))"
echo "SMB_PWRCPLD: $(($pwr_ver)).$(($pwr_sub_ver))"
echo "SCMCPLD: $(($scm_ver)).$(($scm_sub_ver))"
echo "FCMCPLD: $(($fcm_ver)).$(($fcm_sub_ver))"
