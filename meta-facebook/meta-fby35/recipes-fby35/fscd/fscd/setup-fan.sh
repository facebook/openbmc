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

# shellcheck disable=SC1091,SC2039
. /usr/local/fbpackages/utils/ast-functions

default_fsc_config_path="/etc/fsc-config.json"

init_class1_fsc() {
  sys_config=$(/usr/local/bin/show_sys_config | grep -i "config:" | awk -F ": " '{print $3}')
  target_fsc_config=""
  config_type=""
  if [[ "$sys_config" = "A" || "$sys_config" = "C" ]]; then
    config_type="1"
    target_fsc_config="/etc/FSC_CLASS1_type1_config.json"
  elif [ "$sys_config" = "B" ]; then
    config_type="DPV2"
    target_fsc_config="/etc/FSC_CLASS1_DPV2_config.json"
  else
    config_type="1"
    target_fsc_config="/etc/FSC_CLASS1_type1_config.json"
  fi

  ln -s ${target_fsc_config} ${default_fsc_config_path}
  $KV_CMD set sled_system_conf "Type_${config_type}" persistent
}

init_class2_fsc() {
  ln -s /etc/FSC_CLASS2_config.json ${default_fsc_config_path}
  $KV_CMD set sled_system_conf "Type_17" persistent
}

start_sled_fsc() {
  bmc_location=$(get_bmc_board_id)
  if [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
    #The BMC of class1
    init_class1_fsc
  elif [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
    #The BMC of class2
    init_class2_fsc
  else
    echo -n "Is board id correct(id=$bmc_location)?..."
    exit 255
  fi

  echo "Setup fscd for fby35..."
  #Wait for sensord ready
  for _ in {1..5}; do
    sleep 3
    if [ "$(kv get bmc_sensor231)" != "" ]; then
      break
    fi
  done

  runsv /etc/sv/fscd > /dev/null 2>&1 &
}

reload_sled_fsc() {
  bmc_location=$(get_bmc_board_id)
  if [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
    #The BMC of class2 need to check the present status from BB BIC
    slot1_prsnt=$(bic-util slot1 0xe0 0x2 0x9c 0x9c 0x0 0x10 0xe0 0x41 0x9c 0x9c 0x0 0x0 15 | awk '{print $12}')
    slot3_prsnt=$(bic-util slot1 0xe0 0x2 0x9c 0x9c 0x0 0x10 0xe0 0x41 0x9c 0x9c 0x0 0x0 3 | awk '{print $12}')
    if [ "$slot1_prsnt" = "01" ] || [ "$slot3_prsnt" = "01" ]; then
      run_fscd=false
    else
      run_fscd=true
    fi
  else
    cnt=$(get_all_server_prsnt)
    run_fscd=false

    #Check number of slots
    sys_config="$($KV_CMD get sled_system_conf persistent)"
    if [[ "$sys_config" =~ ^(Type_(1|10))$ && "$cnt" -eq 4 ]]; then
      run_fscd=true
    elif [[ "$sys_config" = "Type_DPV2" && "$cnt" -eq 2 ]]; then
      run_fscd=true
    fi
  fi

  fscd_status=$(cat /etc/sv/fscd/supervise/stat)
  if [ $run_fscd = false ]; then
    if [ "$fscd_status" = "run" ]; then
      sleep 1 && /usr/bin/sv stop fscd
      echo "slot is pulled out, stop fscd."
      /usr/local/bin/fan-util --set 100
    else
      echo "fscd is already stopped."
    fi
  else
    #if fscd has been running, there is no need to restart
    if [ "$fscd_status" != "run" ]; then
      sleep 1 && /usr/bin/sv restart fscd
    fi
  fi
}

start_fscd() {
  start_sled_fsc
  #if one of the blades is not present, stop fscd
  reload_sled_fsc
}

case "$1" in
  start)
    echo "Setup fan speed..."
    /usr/local/bin/fan-util --set 70
    (start_fscd &)&
  ;;

  reload)
    #if one of the blades is pulled out, stop fscd
    reload_sled_fsc
  ;;

  *)
    echo "Usage: /etc/init.d/setup-fan.sh {start|reload}"
  ;;
esac

exit 0
