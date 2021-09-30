#! /bin/sh
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

. /usr/local/fbpackages/utils/ast-functions
LOG="/tmp/setup-sic.log"
FEXP_INTF="0x05"
REXP_INTF="0x15"

function run_cmd() {
  echo "bic-util $1" >> $LOG
  /usr/bin/bic-util $1 >> $LOG
}

function init_vishay_ic(){
  local slot=$1
  local intf=$2

  #SiC450
  #OCP: 48A
  run_cmd "$slot 0xe0 0x02 0x9c 0x9c 0x0 $intf 0x18 0x52 0x13 0x28 0x00 0x46 0x60 0xf8"

  #Offset Vout 70mV
  run_cmd "$slot 0xe0 0x02 0x9c 0x9c 0x0 $intf 0x18 0x52 0x13 0x28 0x00 0x22 0x09 0x00"

  #Clear fault flag
  run_cmd "$slot 0xe0 0x02 0x9c 0x9c 0x0 $intf 0x18 0x52 0x13 0x28 0x00 0x03 0x0f"

  #SiC451
  #Offset Vout 50mV
  run_cmd "$slot 0xe0 0x02 0x9c 0x9c 0x0 $intf 0x18 0x52 0x13 0x26 0x00 0x22 0x06 0x00"

  #Clear fault flag
  run_cmd "$slot 0xe0 0x02 0x9c 0x9c 0x0 $intf 0x18 0x52 0x13 0x26 0x00 0x03"
}

function check_class1_config() {
  local slot=$1
  local val=$2
  local i2c_num=$(get_cpld_bus ${slot:4:1})
  local type_2ou=$(get_2ou_board_type $i2c_num)

  if [[ $((val & 0x08)) = "0" && $type_2ou == "0x06" ]]; then
    # waive 2OU present if 2OU type is dp riser card (0x06)
    val=$((val | 0x08))
  fi

  if [ $val = 0 ]; then
    echo "2OU:" >> $LOG
    init_vishay_ic $slot $REXP_INTF 
    echo "1OU:" >> $LOG
    init_vishay_ic $slot $FEXP_INTF 
  elif [ $val = 4 ]; then
    echo "1OU is not present" >> $LOG
    echo "2OU:" >> $LOG
    init_vishay_ic $slot $REXP_INTF 
  elif [ $val = 8 ]; then
    echo "2OU is not present" >> $LOG
    echo "1OU:" >> $LOG
    init_vishay_ic $slot $FEXP_INTF
  else
    echo "1OU and 2OU are not present" >> $LOG
  fi
}

function init_class1_sic(){
  for slot_num in $(seq 1 4); do
    slot_present=$(gpio_get PRSNT_MB_BMC_SLOT${slot_num}_BB_N)
    slot="slot${slot_num}"
    if [[ "$slot_present" == "1" ]]; then
      echo "$slot is not present" >> $LOG
    elif [[ $(is_sb_bic_ready ${slot_num}) == "0" ]]; then
      echo "$slot bic is not ready" >> $LOG
    else
      val=$(get_m2_prsnt_sts $slot)
      echo "$slot" >> $LOG
      check_class1_config $slot $val
    fi
    echo "" >> $LOG
  done
}

function check_4u_gpv3_type() {
  echo "Check 4U GPv3 type" >> $LOG
  # get the top configure
  cmd="slot1 0xe0 0x02 0x9c 0x9c 0x0 0x15 0xe0 0x02 0x9c 0x9c 0x0 0x45 0xe0 0x66 0x9c 0x9c 0x0 0x0"
  run_cmd "$cmd"
  top_cfg=$(cut -d' ' -f2 <<<"$(cat $LOG | tail -n 1)")
  echo "top_cfg result: $top_cfg" >> $LOG

  echo "" >> $LOG

  # get the bot configure
  cmd="slot1 0xe0 0x02 0x9c 0x9c 0x0 0x15 0xe0 0x02 0x9c 0x9c 0x0 0x40 0xe0 0x66 0x9c 0x9c 0x0 0x0"
  run_cmd "$cmd"
  bot_cfg=$(cut -d' ' -f2 <<<"$(cat $LOG | tail -n 1)")
  echo "bot_cfg result: $bot_cfg" >> $LOG

  if ! [ "$bot_cfg" == "$top_cfg" ]; then
    echo "BMC doesn't support multi config" >> $LOG
    echo "Use TOP_CFG ${top_cfg} by default" >> $LOG
  fi

  echo $top_cfg
}

function check_2u_gpv3_type() {
  echo "Check 2U GPv3 type" >> $LOG

  cmd="slot1 0xe0 0x02 0x9c 0x9c 0x0 0x15 0xe0 0x66 0x9c 0x9c 0x0 0x0"
  run_cmd "$cmd"

  cfg=$(cut -d' ' -f11 <<<"$(cat $LOG | tail -n 1)")
  echo -n "result: " >> $LOG
  echo $cfg | tee -a $LOG
}

function init_class2_sic(){
  local slot="slot1"
  local cfg=""

  val=$(get_m2_prsnt_sts $slot)
  echo "$slot" >> $LOG
  if [ $val = 0 ] || [ $val = 4 ]; then
    echo "2OU:" >> $LOG
    # 0h = GPv3 with BRCM
    # 1h = 2OU EXP w/o PCIe Switch
    # 2h = SPE
    # 3h = GPv3 w/ MCHP
    # 4h = CWC w/ MCHP
    # 5h = TBD
    # 6h = DPv1, class 1
    # 7h = DPv2, class 1
    EXP_BOARD_TYPE=$(get_2ou_board_type 4) #only slot1
    if [ $EXP_BOARD_TYPE == "0x01" ]; then
      init_vishay_ic $slot $REXP_INTF
    else
      # GPv3, CWC, and SPE
      echo "no need to initialize it because it's done by BIC or no VRs" >> $LOG
      # check if it's 2U or 4U GPv3
      if [ $EXP_BOARD_TYPE == "0x04" ]; then
        cfg=$(check_4u_gpv3_type)
      elif [ $EXP_BOARD_TYPE == "0x03" ]; then
        cfg=$(check_2u_gpv3_type)
      fi

      [[ ! -z "$cfg" ]] && /usr/bin/kv set "gpv3_cfg" "$cfg" create
    fi
  else
    echo "2OU is not present" >> $LOG
  fi
}

echo "" > $LOG
if [[ $1 =~ slot ]]; then
  val=$(get_m2_prsnt_sts $1)
  echo "$1" >> $LOG
  check_class1_config $1 $val
  exit
fi

echo -n "Setup VishayIC for fby3..."
bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  #The BMC of class2
  init_class2_sic
elif [ $bmc_location -eq 14 ] ||  [ $bmc_location -eq 7 ]; then
  #The BMC of class1
  init_class1_sic
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

echo "Done."
