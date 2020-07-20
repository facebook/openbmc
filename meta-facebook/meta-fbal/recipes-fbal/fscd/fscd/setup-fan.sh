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
# Default-Start:     5
# Default-Stop:
# Short-Description: Set fan speed
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo "Setup fan speed... "
/usr/local/bin/ipmb-util 8 0x68 0xd8 0x08 0x05 0xff

default_fsc_config_path="/etc/fsc-config.json"

host=$(gpio_get FM_BMC_SKT_ID_0)  #Master:host=1, Master:host=0
mode=$(($(gpio_get FM_BMC_SKT_ID_1)<<1 | $(gpio_get FM_BMC_SKT_ID_2))) #8S:mode=00, 2S:mode=1 4S:mode=2

#echo host = $host
#echo mode = $mode

if [ $mode == 1 ]; then
  echo probe 2s fan table
  ln -s /etc/fsc-config2s.json ${default_fsc_config_path}
elif [[ "$mode" -eq 2 ]] && [[ "$host" -eq 0 ]]; then
  echo probe 4s master fan table
  ln -s /etc/fsc-config4s_M.json ${default_fsc_config_path}
elif [[ "$mode" -eq 0 ]] && [[ "$host" -eq 0 ]]; then
  echo probe 8s master fan table
  ln -s /etc/fsc-config8s_M.json ${default_fsc_config_path}
else
  echo probe slave fan table
  ln -s /etc/fsc-config_Slave.json ${default_fsc_config_path}
fi

runsv /etc/sv/fscd > /dev/null 2>&1 &
echo "done."
