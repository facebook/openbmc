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

default_fsc_config_path="/etc/fsc-config.json"

function init_class1_fsc(){
  sys_config=$(/usr/local/bin/show_sys_config | grep -i "config:" | awk -F ": " '{print $3}')
  fan0_type=$(gpio_get DUAL_FAN0_DETECT_BMC_N_R)
  fan1_type=$(gpio_get DUAL_FAN1_DETECT_BMC_N_R)

  if [ $fan0_type -ne $fan1_type ]; then
    echo "fan type is not the same!"
  fi

  if [ $fan0_type -eq 1 ] && [ "$sys_config" = "A" ]; then
    ln -s /etc/FSC_CLASS1_EVT_type1.json ${default_fsc_config_path}
  elif [ $fan0_type -eq 0 ] && [ "$sys_config" = "A" ]; then
    ln -s /etc/FSC_CLASS1_EVT_dualfan_type1.json ${default_fsc_config_path}
  elif [ $fan0_type -eq 1 ] && [ "$sys_config" = "B" ]; then
    ln -s /etc/FSC_CLASS1_EVT_type10.json ${default_fsc_config_path}
  else
    ln -s /etc/FSC_CLASS1_EVT_dualfan_type10.json ${default_fsc_config_path}
  fi
}

function init_class2_fsc(){
  ln -s /etc/FSC_CLASS2_EVT_config.json ${default_fsc_config_path}
}

bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  #The BMC of class2
  init_class2_fsc
elif [ $bmc_location -eq 14 ]; then
  #The BMC of class1
  init_class1_fsc
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

echo "Setup fan speed..."
/usr/local/bin/fan-util --set 70
echo -n "Setup fscd for fby3..."
runsv /etc/sv/fscd > /dev/null 2>&1 &
echo "Done."
