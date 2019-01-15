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

board_rev=$(wedge_board_rev)

smb_ver=`head -n 1 /sys/class/i2c-adapter/i2c-12/12-003e/cpld_ver 2> /dev/null`
smb_sub_ver=`head -n 1 /sys/class/i2c-adapter/i2c-12/12-003e/cpld_sub_ver 2> /dev/null`
if [ $? -eq 0 ]; then
  echo "SMBCPLD: $(($smb_ver)).$(($smb_sub_ver))"
else
  echo "SMBCPLD is not detected"
fi
usleep 50000

scm_ver=`head -n 1 /sys/class/i2c-adapter/i2c-2/2-0035/cpld_ver 2> /dev/null`
scm_sub_ver=`head -n 1 /sys/class/i2c-adapter/i2c-2/2-0035/cpld_sub_ver 2> /dev/null`
if [ $? -eq 0 ]; then
  echo "SCMCPLD: $(($scm_ver)).$(($scm_sub_ver))"
else
  echo "SCMCPLD is not detected"
fi
usleep 50000

fcm_top_ver=`head -n 1 /sys/class/i2c-adapter/i2c-64/64-0033/cpld_ver 2> /dev/null`
fcm_top_sub_ver=`head -n 1 /sys/class/i2c-adapter/i2c-64/64-0033/cpld_sub_ver 2> /dev/null`
if [ $? -eq 0 ]; then
  echo "Top FCMCPLD: $(($fcm_top_ver)).$(($fcm_top_sub_ver))"
else
  echo "Top FCMCPLD is not detected"
fi
usleep 50000

fcm_bottom_ver=`head -n 1 /sys/class/i2c-adapter/i2c-72/72-0033/cpld_ver 2> /dev/null`
fcm_bottom_sub_ver=`head -n 1 /sys/class/i2c-adapter/i2c-72/72-0033/cpld_sub_ver 2> /dev/null`
if [ $? -eq 0 ]; then
  echo "Bottom FCMCPLD: $(($fcm_bottom_ver)).$(($fcm_bottom_sub_ver))"
else
  echo "Bottom FCMCPLD is not detected"
fi
usleep 50000

if [ $board_rev -ne 4 ]; then
  pdbl_ver=`head -n 1 /sys/class/i2c-adapter/i2c-55/55-0060/cpld_ver 2> /dev/null`
  pdbl_sub_ver=`head -n 1 /sys/class/i2c-adapter/i2c-55/55-0060/cpld_sub_ver 2> /dev/null`
  if [ $? -eq 0 ]; then
    echo "Left PDBCPLD: $(($pdbl_ver)).$(($pdbl_sub_ver))"
  else
    echo "Left PDBCPLD is not detected"
  fi
  usleep 50000

  pdbr_ver=`head -n 1 /sys/class/i2c-adapter/i2c-63/63-0060/cpld_ver 2> /dev/null`
  pdbr_sub_ver=`head -n 1 /sys/class/i2c-adapter/i2c-63/63-0060/cpld_sub_ver 2> /dev/null`
  if [ $? -eq 0 ]; then
    echo "Right PDBCPLD: $(($pdbr_ver)).$(($pdbr_sub_ver))"
  else
    echo "Right PDBCPLD is not detected"
  fi
fi
