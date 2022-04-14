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
    target_fsc_config="/etc/FSC_CLASS1_PVT_type1.json"
  elif [ "$sys_config" = "B" ]; then
    get_1ou_type
    if [ "$type_1ou" = "EDSFF_1U" ]; then
      echo "use EDSFF_1U fan table"
      config_type="EDSFF_1U"
      target_fsc_config="/etc/FSC_CLASS1_DVT_EDSFF_1U.json"
    else
      config_type="10"
      target_fsc_config="/etc/FSC_CLASS1_EVT_type10.json"
    fi
  elif [ "$sys_config" = "D" ]; then
    get_2ou_type
    config_type="15"
    if ([ $type_2ou == "0x00" ] || [ $type_2ou == "0x03" ]); then
      echo "use Config D GPv3 fan table"
      target_fsc_config="/etc/FSC_CLASS1_CONFIG_D_GPV3.json"
    elif [ "$type_2ou" == "0x06" ]; then
      echo "use DP fan table"
      config_type="DP"
      target_fsc_config="/etc/FSC_CLASS1_EVT_DP.json"
    else
      target_fsc_config="/etc/FSC_CLASS1_type15.json"
    fi
  else
    config_type="15"
    target_fsc_config="/etc/FSC_CLASS1_type15.json"
  fi

  ln -s ${target_fsc_config} ${default_fsc_config_path}
  echo -n "Type_${config_type}" > /mnt/data/kv_store/sled_system_conf
}

function init_class2_fsc(){
  cpld_bus_num="4"
  exp_board=$(get_2ou_board_type $cpld_bus_num)
  if [ "$exp_board" == "0x02" ]; then # E1.S board
    ln -s /etc/FSC_CLASS2_PVT_SPE_config.json ${default_fsc_config_path}
    echo -n "Type_8" > /mnt/data/kv_store/sled_system_conf
  elif [ "$exp_board" == "0x04" ]; then #CWC board
    ln -s /etc/FSC_CLASS2_DVT_CWC.json ${default_fsc_config_path}
    echo -n "Type_17" > /mnt/data/kv_store/sled_system_conf
  else
    ln -s /etc/FSC_CLASS2_EVT_config.json ${default_fsc_config_path}
    echo -n "Type_17" > /mnt/data/kv_store/sled_system_conf
  fi
  
}


function get_1ou_type(){
  for i in $(seq 1 4)
  do
    if [[ $(is_sb_bic_ready $i) == "1" ]]; then
      output=$(bic-util slot$i 0xE0 2 0x9c 0x9c 0 0x5 0xe0 0xa0 0x9c 0x9c 0 | awk '{print $7" "$11}')
      if [[ ${output} == "00 07" ]]; then
        type_1ou=EDSFF_1U
        break
      elif [[ ${output} == "C1 " ]]; then
        break
      fi
    fi
  done
}

function get_2ou_type(){
  for i in {1..4..2}
  do
    I2C_NUM=$(($i+3))
    type_2ou=$(get_2ou_board_type $I2C_NUM)
    if [ $type_2ou != "0xff" ]; then
      break
    fi
  done
}

function start_sled_fsc() {
  echo "Setup fan speed..."
  /usr/local/bin/fan-util --set 70

  bmc_location=$(get_bmc_board_id)
  if [ $bmc_location -eq 9 ]; then
    #The BMC of class2
    init_class2_fsc
  elif [ $bmc_location -eq 14 ] || [ $bmc_location -eq 7 ]; then
    #The BMC of class1
    init_class1_fsc
  else
    echo -n "Is board id correct(id=$bmc_location)?..."
    exit -1
  fi

  echo -n "Setup fscd for fby3..."
  runsv /etc/sv/fscd > /dev/null 2>&1 &
  echo "Done."
}

function reload_sled_fsc() {
  bmc_location=$(get_bmc_board_id)
  if [ $bmc_location -eq 9 ]; then
    exp_board=$(get_2ou_board_type 4)

    if [ $exp_board == "0x04" ]; then
      run_fscd=true
    else
      #The BMC of class2 need to check the present status from BB BIC
      slot1_prsnt=$(bic-util slot1 0xe0 0x2 0x9c 0x9c 0x0 0x10 0xe0 0x41 0x9c 0x9c 0x0 0x0 27 | awk '{print $12}')
      slot3_prsnt=$(bic-util slot1 0xe0 0x2 0x9c 0x9c 0x0 0x10 0xe0 0x41 0x9c 0x9c 0x0 0x0 28 | awk '{print $12}')
      if [ "$slot1_prsnt" == "01" ] || [ "$slot3_prsnt" == "01" ]; then
        run_fscd=false
      else
        run_fscd=true
      fi
    fi
  else
    cnt=`get_all_server_prsnt`
    run_fscd=false
  
    #Config A and B or Config D
    sys_config=$(cat /mnt/data/kv_store/sled_system_conf)
    if [[ "$sys_config" =~ ^(Type_(1|10|EDSFF_1U))$ ]]; then
      if [ $cnt -eq 4 ]; then
        run_fscd=true
      fi
    else
      if [ $cnt -eq 2 ]; then
        run_fscd=true
      elif [ $cnt -eq 1 ]; then
        # DP system only has one slot
        if [ "$sys_config" = "Type_DP" ] || [ "$sys_config" = "Type_DPB" ] || [ "$sys_config" = "Type_DPF" ]; then
          run_fscd=true
        fi
      fi
    fi
  fi

  fscd_status=$(cat /etc/sv/fscd/supervise/stat)
  if [ $run_fscd = false ]; then
    if [ "$fscd_status" == "run" ]; then
      sleep 1 && /usr/bin/sv stop fscd
      echo "slot is pulled out, stop fscd."
      /usr/local/bin/fan-util --set 100
    else
      echo "fscd is already stopped."
    fi
  else
    #if fscd has been running, there is no need to restart
    if ! [ "$fscd_status" == "run" ]; then
      sleep 1 && /usr/bin/sv restart fscd
    fi
  fi
}

case "$1" in
  start)
    start_sled_fsc
    #if one of the blades is not present, stop fscd
    reload_sled_fsc
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
