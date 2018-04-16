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

smb_ver=$(cat /sys/class/i2c-adapter/i2c-12/12-003e/cpld_ver)
smb_sub_ver=$(cat /sys/class/i2c-adapter/i2c-12/12-003e/cpld_sub_ver)
scm_ver=$(cat /sys/class/i2c-adapter/i2c-2/2-0035/cpld_ver)
scm_sub_ver=$(cat /sys/class/i2c-adapter/i2c-2/2-0035/cpld_sub_ver)
fcm_top_ver=$(cat /sys/class/i2c-adapter/i2c-64/64-0033/cpld_ver)
fcm_top_sub_ver=$(cat /sys/class/i2c-adapter/i2c-64/64-0033/cpld_sub_ver)
fcm_bottom_ver=$(cat /sys/class/i2c-adapter/i2c-72/72-0033/cpld_ver)
fcm_bottom_sub_ver=$(cat /sys/class/i2c-adapter/i2c-72/72-0033/cpld_sub_ver)

echo "SMBCPLD: $(($smb_ver)).$(($smb_sub_ver))"
echo "SCMCPLD: $(($scm_ver)).$(($scm_sub_ver))"
echo "Top FCMCPLD: $(($fcm_top_ver)).$(($fcm_top_sub_ver))"
echo "Bottom FCMCPLD: $(($fcm_bottom_ver)).$(($fcm_bottom_sub_ver))"

