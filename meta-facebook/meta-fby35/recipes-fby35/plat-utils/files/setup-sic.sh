#! /bin/bash
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
# Provides:          setup Vishay IC 450/451
# Required-Start:
# Required-Stop:
# Default-Start:
# Default-Stop:
# Short-Description: setup Vout and OCP dynamically 
#
### END INIT INFO

# shellcheck disable=SC1091
. /usr/local/fbpackages/utils/ast-functions

LOG="/tmp/setup-sic.log"
FEXP_INTF="0x05"
REXP_INTF="0x15"
IS_INIT_NEED=true

function run_cmd() {
  echo "bic-util $*" >> $LOG
  /usr/bin/bic-util "$@" >> $LOG
}

function init_vishay_gpv3_ic() {
  local slot=$1
  local intf=$2
  local VR_STBY1_ADDR="0x28"
  local VR_STBY2_ADDR="0x2E"
  local VR_STBY3_ADDR="0x30"
  for addr in $VR_STBY1_ADDR $VR_STBY2_ADDR $VR_STBY3_ADDR; do
    #Offset Vout 70mV
    echo "$addr" >> $LOG
    run_cmd "$slot" 0xe0 0x02 0x9c 0x9c 0x0 "$intf" 0x18 0x52 0x01 "$addr" 0x00 0x22 0x09 0x00
    run_cmd "$slot" 0xe0 0x02 0x9c 0x9c 0x0 "$intf" 0x18 0x52 0x01 "$addr" 0x00 0x03
    echo >> $LOG
  done

  # enable VR fault monitor on BIC
  run_cmd "$slot" 0xe0 0x2 0x9c 0x9c 0x0 0x15 0xe0 0x69 0x9c 0x9c 0x0 0x1
}

function init_vishay_ic() {
  local slot=$1
  local intf=$2

  #SiC450
  #OCP: 48A
  run_cmd "$slot" 0xe0 0x02 0x9c 0x9c 0x0 "$intf" 0x18 0x52 0x11 0x28 0x00 0x46 0x60 0xf8

  #Offset Vout 70mV
  run_cmd "$slot" 0xe0 0x02 0x9c 0x9c 0x0 "$intf" 0x18 0x52 0x11 0x28 0x00 0x22 0x09 0x00

  #Clear fault flag
  run_cmd "$slot" 0xe0 0x02 0x9c 0x9c 0x0 "$intf" 0x18 0x52 0x11 0x28 0x00 0x03 0x0f

  #SiC451
  #Offset Vout 50mV
  run_cmd "$slot" 0xe0 0x02 0x9c 0x9c 0x0 "$intf" 0x18 0x52 0x11 0x26 0x00 0x22 0x06 0x00

  #Clear fault flag
  run_cmd "$slot" 0xe0 0x02 0x9c 0x9c 0x0 "$intf" 0x18 0x52 0x11 0x26 0x00 0x03
}

function check_class1_config() {
  local slot=$1
  local val=$2
  local i2c_num
  local type_2ou

  i2c_num=$(get_cpld_bus "${slot:4:1}")
  type_2ou=$(get_2ou_board_type "$i2c_num")

  if [[ $((val & 0x08)) -eq 0 && 
    ($((type_2ou & 0x07)) -eq 0x07 || $((type_2ou & 0x70)) -eq 0x70) ]]; then
    # waive 2OU present if 2OU type is dpv2 riser card X8 (0x07) / X16 (0x70)
    val=$((val | 0x08))
    IS_INIT_NEED=false
  fi

  if [ $val -eq 0 ]; then
    echo "2OU:" >> $LOG
    init_vishay_ic "$slot" $REXP_INTF
    echo "1OU:" >> $LOG
    init_vishay_ic "$slot" $FEXP_INTF
  elif [ $val -eq 4 ]; then
    echo "1OU is not present" >> $LOG
    echo "2OU:" >> $LOG
    init_vishay_ic "$slot" $REXP_INTF
  elif [ $val -eq 8 ]; then
    echo "2OU is not present" >> $LOG
    echo "1OU:" >> $LOG
    init_vishay_ic "$slot" $FEXP_INTF
  else
    echo "1OU and 2OU are not present" >> $LOG
  fi
}

function init_class1_sic() {
  for slot_num in {1..4}; do
    if [[ $slot_num -eq 1 || $slot_num -eq 3 ]]; then
      prsnt_sts=$(gpio_get PRSNT_MB_BMC_SLOT${slot_num}_BB_N)
    else
      prsnt_sts=$(gpio_get PRSNT_MB_SLOT${slot_num}_BB_N)
    fi
    slot="slot${slot_num}"
    if [ "$prsnt_sts" == "1" ]; then
      echo "$slot is not present" >> $LOG
    elif [ "$(is_sb_bic_ready ${slot_num})" == "0" ]; then
      echo "$slot bic is not ready" >> $LOG
    else
      val=$(get_m2_prsnt_sts $slot)
      echo "$slot" >> $LOG
      check_class1_config "$slot" "$val"
    fi
    echo "" >> $LOG
  done
}

function init_class2_sic() {
  local slot="slot1"
  val=$(get_m2_prsnt_sts $slot)
  echo "$slot" >> $LOG
  if [[ "$val" -eq 0 || "$val" -eq 4 ]]; then
    echo "2OU:" >> $LOG
    EXP_BOARD_TYPE=$(get_2ou_board_type 4) #only slot1
    if [[ "$EXP_BOARD_TYPE" == "0x00" || "$EXP_BOARD_TYPE" == "0x03" ]]; then
      init_vishay_gpv3_ic $slot $REXP_INTF
    else
      init_vishay_ic $slot $REXP_INTF
    fi
  else
    echo "2OU is not present" >> $LOG
  fi
}

echo "" > $LOG
if [[ $1 =~ slot ]]; then
  val=$(get_m2_prsnt_sts "$1")
  echo "$1" >> $LOG
  check_class1_config "$1" "$val"
  exit
fi

echo -n "Setup VishayIC for fby35..."
bmc_location=$(get_bmc_board_id)
if [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
  #The BMC of class1
  init_class1_sic
elif [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
  #The BMC of class2
  init_class2_sic
else
  echo -n "Is board id correct(id=$bmc_location)?..."
  exit 255
fi

if [[ $IS_INIT_NEED == true ]]; then
  echo "init done."
else
  echo "init not needed."
fi
