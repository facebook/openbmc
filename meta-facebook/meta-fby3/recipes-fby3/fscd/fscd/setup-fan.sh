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
  target_fsc_config=""
  config_type=""
  if [ "$sys_config" = "A" ]; then
    config_type="1"
    target_fsc_config="/etc/FSC_CLASS1_EVT_type1.json"
  elif [ "$sys_config" = "B" ]; then
    config_type="10"
    target_fsc_config="/etc/FSC_CLASS1_EVT_type10.json"
  else
    config_type="15"
    target_fsc_config="/etc/FSC_CLASS1_type15.json"
  fi

  ln -s ${target_fsc_config} ${default_fsc_config_path}
  echo -n "Type_${config_type}" > /mnt/data/kv_store/sled_system_conf
}

function init_class2_fsc(){
  ln -s /etc/FSC_CLASS2_EVT_config.json ${default_fsc_config_path}
  echo -n "Type_17" > /mnt/data/kv_store/sled_system_conf
}

bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  #The BMC of class2
  init_class2_fsc
elif [ $bmc_location -eq 14 ] || [ $bmc_location -eq 7 ]; then
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
