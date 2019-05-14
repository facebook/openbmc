#!/bin/sh
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

default_fsc_config="/etc/fsc-config.json"
fcb_ver=`head -n1 $FCMCPLD_SYSFS_DIR/cpld_ver \
         2> /dev/null`
fcb_sub_ver=`head -n1 $FCMCPLD_SYSFS_DIR/cpld_sub_ver \
         2> /dev/null`
fcb_maj=`echo $fcb_ver | awk '{printf "%d", $1;}'`
fcb_min=`echo $fcb_sub_ver | awk '{printf "%d", $1;}'`
echo -n "Setup fan speed... "

if [ "$fcb_ver" == "0x0" ]; then
    echo "Run FSC PWM 32 Levels Config"
    cp /etc/FSC-PWM-32-config.json ${default_fsc_config}
else
    echo "Run FSC PWM 64 Levels Config"
    cp /etc/FSC-PWM-64-config.json ${default_fsc_config}
fi
echo "Setting fan speed to 25%..."
/usr/local/bin/set_fan_speed.sh 25
echo "Done setting fan speed"
# Currently, fcbcpld version 0.xx will cause BMC to 
# run into crash loop, due to fscd's own logic and 
# its failure to disarm bootup watchdog.
# (this problem is only for fcbcpld 0.xx. Fcmcpld 1.9 and 
# above works well without any problem.)
# Until we have fix, we will run fscd only if fcbcpld is 1.9 or higher.
fcb_compatible=0
if [ $fcb_maj -eq 1 ] && [ $fcb_min -ge 9 ]; then 
   fcb_compatible=1
fi
if [ $fcb_maj -ge 2 ]; then
   fcb_compatible=1
fi

if [ $(wedge_board_rev) -eq 4 ]; then
    fcb_compatible=0
fi

if [ $fcb_compatible -eq 0 ]; then
    echo "Running fan at the fixed speed."
    /usr/local/bin/wdtcli stop
else
    echo "Starting fscd..."
    runsv /etc/sv/fscd > /dev/null 2>&1 &
fi
echo "done."
