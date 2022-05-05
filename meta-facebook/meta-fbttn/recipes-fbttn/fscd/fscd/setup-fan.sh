#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
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

echo -n "Setup fscd config... "

default_fsc_config_path="/etc/fsc-config.json"
fsc_type5_config_path="/etc/FSC_BC_Type5_MP_v4_config.json"
fsc_type7_config_path="/etc/FSC_BC_Type7_MP_v4_config.json"
IOM_M2=1      # IOM type: M.2
IOM_IOC=2     # IOM type: IOC

sh /usr/local/bin/check_pal_sku.sh > /dev/NULL
  PAL_SKU=$(($? & 0x3))
  if [ "$PAL_SKU" == "$IOM_M2" ]; then
    cp ${fsc_type5_config_path} ${default_fsc_config_path}
  elif [ "$PAL_SKU" == "$IOM_IOC" ]; then
  	cp ${fsc_type7_config_path} ${default_fsc_config_path}
  else
  	cp ${fsc_type5_config_path} ${default_fsc_config_path}
  	echo "Setup fscd: System type is unknown, using Type5 Config! Please confirm the system type."
  fi

/usr/local/bin/init_pwm.sh
/usr/local/bin/fan-util --set 50
runsv /etc/sv/fscd > /dev/null 2>&1 &
echo "done."
