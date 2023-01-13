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

TYPE_DUAL_FAN=0
TYPE_SINGEL_FAN=1
GET_FAN_RPM="/usr/bin/bic-util slot1 0xe0 0x2 ""$IANA_ID"" 0x10 0xe0 0x52 ""$IANA_ID" # for config D
default_fsc_config_path="/etc/fsc-config.json"
sys_config=$(/usr/local/bin/show_sys_config | grep -i "config:" | awk -F ": " '{print $3}')
bmc_location=$(get_bmc_board_id)

#For config A B C, we get the Baseboard revision from BB CPLD through i2cget.
CLASS1_GET_BOARD_REVID="/usr/sbin/i2cget -y 12 0x0f 0x08"
#For config D, we get the Baseboard revision from BB BPLD. command: bic-util slot1 IPMI Send request message to BIC, IANA, BB bic, IPMI Master write read, CPLD i2cbus addr register
CLASS2_GET_BOARD_REVID="/usr/bin/bic-util slot1 0xe0 0x2 ""$IANA_ID"" 0x10 0x18 0x52 0x1 0x1e 0x1 0x8 "

check_dvt_fan(){
  local fan_type=$1
  if [ "$fan_type" = "$TYPE_DUAL_FAN" ]; then
    logger -t "fan_check" -p daemon.crit "Fan slot $i is dual fan, please install single fan in DVT system"
  fi
}

check_evt_fan(){
  local fan_type=$1
  if [ "$fan_type" = "$TYPE_DUAL_FAN" ]; then
    if [ "$sys_config" = "A" ]; then
      logger -t "fan_check" -p daemon.crit "Fan slot $i is dual fan, please install single fan on 1U system"
    fi
  else
    if [[ "$sys_config" = "B" || "$sys_config" = "D" ]]; then
      logger -t "fan_check" -p daemon.crit "Fan slot $i is single fan, please install dual fan on 2U system"
    fi
  fi
}

check_fan_type() {
  if [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
    # check CC of from BIC, if there is single fan TACH 1, 3, 5, 7 will not turn CC "00"
    if [[ "$($GET_FAN_RPM 1 | awk '{print $7;}')" = "00" && "$($GET_FAN_RPM 3 | awk '{print $7;}')" = "00" ]]; then
      fan0_type=$TYPE_DUAL_FAN
    else
      fan0_type=$TYPE_SINGEL_FAN
    fi
    if [[ "$($GET_FAN_RPM 5 | awk '{print $7;}')" = "00" && "$($GET_FAN_RPM 7 | awk '{print $7;}')" = "00" ]]; then
      fan1_type=$TYPE_DUAL_FAN
    else
      fan1_type=$TYPE_SINGEL_FAN
    fi
  else
    if [[ "$(cat /sys/class/hwmon/hwmon*/fan2_input)" = "0" && "$(cat /sys/class/hwmon/hwmon*/fan4_input)" = "0" ]]; then
      fan0_type=$TYPE_SINGEL_FAN
    else
      fan0_type=$TYPE_DUAL_FAN
    fi
    if [[ "$(cat /sys/class/hwmon/hwmon*/fan6_input)" = "0" && "$(cat /sys/class/hwmon/hwmon*/fan8_input)" = "0" ]]; then
      fan1_type=$TYPE_SINGEL_FAN
    else
      fan1_type=$TYPE_DUAL_FAN
    fi
  fi
  fan_type=("$fan0_type" "$fan1_type")
  for ((i=0; i < ${#fan_type[@]}; i++))
  do
    if [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
      BOARD_REVID=$($CLASS1_GET_BOARD_REVID)
      if [ "$((0x${BOARD_REVID#*x}))" -ge "$DVT_REVID" ]; then # for revision id >= 5 means the stage is DVT or after DVT
        check_dvt_fan "${fan_type[$i]}"
      else
        check_evt_fan "${fan_type[$i]}"
      fi
    elif [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
      BOARD_REVID=$($CLASS2_GET_BOARD_REVID | awk '{print $8;}')
      if [ "$((0x${BOARD_REVID#*x}))" -ge "$DVT_REVID" ]; then
        check_dvt_fan "${fan_type[$i]}"
      else
        check_evt_fan "${fan_type[$i]}"
      fi
    else
      logger -t "fan_check" -p daemon.crit "Cannot get the location of BMC"
    fi
  done
}

init_class1_fsc() {
  sys_config=$(/usr/local/bin/show_sys_config | grep -i "config:" | awk -F ": " '{print $3}')
  target_fsc_config=""
  config_type=""
  if [ "$sys_config" = "A" ]; then
    server_type=$(get_server_type 1)
    if [[ $server_type -eq 4 ]]; then
      config_type="GL"
      target_fsc_config="/etc/FSC_CLASS1_GL_config.json"
    else
      config_type="1"
      target_fsc_config="/etc/FSC_CLASS1_type1_config.json"
    fi
  elif [ "$sys_config" = "B" ]; then
    server_type=$(get_server_type 1)
    if [[ $server_type -eq 2 ]]; then
      board_type_1ou=$(get_1ou_board_type slot1)
      case $board_type_1ou in
      8)
        config_type="8"
        target_fsc_config="/etc/FSC_CLASS1_type8_config.json"
        ;;
      *)
        config_type="HD"
        target_fsc_config="/etc/FSC_CLASS1_HD_config.json"
        ;;
      esac
    else
      config_type="DPV2"
      target_fsc_config="/etc/FSC_CLASS1_DPV2_config.json"
    fi
  elif [ "$sys_config" = "C" ]; then
    server_type=$(get_server_type 1)
    if [[ $server_type -eq 4 ]]; then
      config_type="GL"
      target_fsc_config="/etc/FSC_CLASS1_GL_config.json"
    else
      board_type_1ou=$(get_1ou_board_type slot1)
      case $board_type_1ou in
        4)
          config_type="VF"
          target_fsc_config="/etc/FSC_CLASS1_type3_10_config.json"
          ;;
        *)
          logger -t "fan_check" -p daemon.crit "Fail to get server type, run with the default fan config"
          config_type="1"
          target_fsc_config="/etc/FSC_CLASS1_type1_config.json"
          ;;
      esac
    fi
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
  check_fan_type

  echo "Setup fscd for fby35..."
  #Wait for sensord ready
  for _ in {1..5}; do
    sleep 3
    if [ "$($KV_CMD get bmc_sensor231)" != "" ]; then
      break
    fi
  done

  runsv /etc/sv/fscd > /dev/null 2>&1 &
}

reload_sled_fsc() {
  bmc_location=$(get_bmc_board_id)
  if [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
    #The BMC of class2 need to check the present status from BB BIC
    slot1_prsnt=$(bic-util slot1 0xe0 0x2 "$IANA_ID" 0x10 0xe0 0x41 "$IANA_ID" 0x0 15 | awk '{print $12}')
    slot3_prsnt=$(bic-util slot1 0xe0 0x2 "$IANA_ID" 0x10 0xe0 0x41 "$IANA_ID" 0x0 3 | awk '{print $12}')
    if [ "$slot1_prsnt" = "01" ] || [ "$slot3_prsnt" = "01" ]; then
      run_fscd=false
    else
      run_fscd=true
    fi
  else
    cnt=$(get_all_server_prsnt)

    #Check number of slots
    sys_config="$($KV_CMD get sled_system_conf persistent)"
    if [[ "$sys_config" =~ ^(Type_(1|10|VF|GL))$ && "$cnt" -eq 4 ]]; then
      run_fscd=true
    elif [[ "$sys_config" =~ ^(Type_(DPV2|HD))$ && "$cnt" -eq 2 ]]; then
      run_fscd=true
    elif [[ "$sys_config" == "Type_8" && "$cnt" -eq 1 ]]; then
      run_fscd=true
    else
      run_fscd=false
    fi
  fi

  if [ $run_fscd = false ]; then
    echo "mismatched system config, boost fans!"
    $KV_CMD set fan_mode_event 2 # boost_mode
  else
    $KV_CMD del fan_mode_event 2>/dev/null
  fi

  fscd_status=$(cat /etc/sv/fscd/supervise/stat)
  if [ "$fscd_status" != "run" ]; then
    sleep 1 && /usr/bin/sv restart fscd
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
