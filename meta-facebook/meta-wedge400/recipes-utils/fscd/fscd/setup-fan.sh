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

### BEGIN INIT INFO
# Provides:          setup-fan
# Required-Start:    board-id
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set fan speed
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions
. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# if fcm version or higher, enable fscd
FSCD_ENABLE_FCM_MAJ=3
FSCD_ENABLE_FCM_MIN=5

default_fsc_config="/etc/fsc-config.json"
fcm_maj=$((FCM_CPLD_VER))
fcm_min=$((FCM_CPLD_SUB_VER))

echo -n "Setup fan speed... "

if [ "$(wedge_board_type)" = "0" ]; then
    echo "Run FSC PWM 64 Levels Config"
    cp /etc/FSC-W400-config.json ${default_fsc_config}
fi
echo "Setting fan speed to 50%..."
/usr/local/bin/set_fan_speed.sh 50
echo "Done setting fan speed"
# Currently, fcmcpld version 0.xx will cause BMC to 
# run into crash loop, due to fscd's own logic and 
# its failure to disarm bootup watchdog.
# (this problem is only for fcmcpld 0.xx. Fcmcpld 3.5 and
# above works well without any problem.)
# Until we have fix, we will run fscd only if fcmcpld is 3.5 or higher.
fcm_compatible=0
if [ $((fcm_maj)) -eq $FSCD_ENABLE_FCM_MAJ ] &&
   [ $((fcm_min)) -ge $FSCD_ENABLE_FCM_MIN ] ||
   [ $((fcm_maj)) -ge $((FSCD_ENABLE_FCM_MAJ + 1)) ]; then
   fcm_compatible=1
fi

#WEDGE400 EVT   wedge_board_rev = 0  smb_board_rev = 00 disable
#WEDGE400 EVT3  wedge_board_rev = 0  smb_board_rev = 01 enable
if [ "$(wedge_board_type)" = "0" ] &&
   [ "$(wedge_board_rev)" = "0" ] &&
   [ "$SMB_CPLD_BOARD_REV" = "0x0" ]; then #WEDGE400 EVT
    fcm_compatible=0
#WEDGE400-C EVT wedge_board_rev = 0  disable
elif [ "$(wedge_board_type)" = "1" ]; then #WEDGE400C
    fcm_compatible=0 #fcm_compatible = 0 means fscd not ready.
fi

if [ $fcm_compatible -eq 0 ]; then
    echo "Running fan at the fixed speed."
    /usr/local/bin/wdtcli stop
else
    echo "Starting fscd..."
    runsv /etc/sv/fscd > /dev/null 2>&1 &
fi
echo "done."
