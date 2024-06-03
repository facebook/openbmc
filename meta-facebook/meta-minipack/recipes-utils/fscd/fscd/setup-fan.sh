#!/bin/sh
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

### BEGIN INIT INFO
# Provides:          setup-fan
# Required-Start:    board-id
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set fan speed
### END INIT INFO

. /usr/local/bin/openbmc-utils.sh
. /usr/local/fbpackages/utils/ast-functions

default_fsc_config="/etc/fsc-config.json"
fcm_b_ver=`head -n1 ${BOTTOM_FCMCPLD_SYSFS_DIR}/cpld_ver \
           2> /dev/null`
fcm_b_sub_ver=`head -n1 ${BOTTOM_FCMCPLD_SYSFS_DIR}/cpld_sub_ver \
           2> /dev/null`
fcm_t_ver=`head -n1 ${TOP_FCMCPLD_SYSFS_DIR}/cpld_ver \
           2> /dev/null`
fcm_t_sub_ver=`head -n1 ${TOP_FCMCPLD_SYSFS_DIR}/cpld_sub_ver \
           2> /dev/null`
fcm_b_maj=`echo $fcm_b_ver | awk '{printf "%d", $1;}'`
fcm_b_min=`echo $fcm_b_sub_ver | awk '{printf "%d", $1;}'`
fcm_t_maj=`echo $fcm_t_ver | awk '{printf "%d", $1;}'`
fcm_t_min=`echo $fcm_t_sub_ver | awk '{printf "%d", $1;}'`
echo -n "Setup fan speed... "

if [ "$fcm_b_ver" == "0x0" ] || [ "$fcm_t_ver" == "0x0" ]; then
    echo "Run FSC PWM 32 Levels Config"
    cp /etc/FSC-PWM-32-config.json ${default_fsc_config}
else
    echo "Run FSC PWM 64 Levels Config"
    cp /etc/FSC-PWM-64-config.json ${default_fsc_config}
fi
echo "Setting fan speed to 50%..."
/usr/local/bin/set_fan_speed.sh 50
echo "Done setting fan speed"
# Currently, fcmcpld version 0.xx will cause BMC to 
# run into crash loop, due to fscd's own logic and 
# its failure to disarm bootup watchdog.
# (this problem is only for fcmcpld 0.xx. Fcmcpld 1.9 and 
# above works well without any problem.)
# Until we have fix, we will run fscd only if fcmcpld is 1.9 or higher.
fcm_b_compatible=0
fcm_t_compatible=0
if [ $fcm_b_maj -eq 1 ] && [ $fcm_b_min -ge 9 ]; then 
   fcm_b_compatible=1
fi
if [ $fcm_b_maj -ge 2 ]; then
   fcm_b_compatible=1
fi
if [ $fcm_t_maj -eq 1 ] && [ $fcm_t_min -ge 9 ]; then
   fcm_t_compatible=1
fi
if [ $fcm_b_maj -ge 2 ]; then
   fcm_t_compatible=1
fi
echo fcm_b version : $fcm_b_maj $fcm_b_min compatible: $fcm_b_compatible
echo fcm_t version : $fcm_t_maj $fcm_t_min compatible: $fcm_t_compatible
if [ $fcm_b_compatible -eq 0 ] || [ $fcm_t_compatible -eq 0 ]; then
    echo "Old FCM CPLD detected. Running fan at the fixed speed."
    /usr/local/bin/wdtcli stop
else
    echo "Starting fscd..."
    runsv /etc/sv/fscd > /dev/null 2>&1 &
fi
echo "done."
