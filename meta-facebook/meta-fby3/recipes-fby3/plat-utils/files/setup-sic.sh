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
}

function check_class1_config() {
  local slot=$1
  local val=$2
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
    if [[ "$slot_present" == "0" ]]; then
      val=$(get_m2_prsnt_sts $slot)
      echo "$slot" >> $LOG
      check_class1_config $slot $val
    else
      echo "$slot is not present" >> $LOG
    fi
    echo "" >> $LOG
  done
}

function init_class2_sic(){
  local slot="slot1"
  val=$(get_m2_prsnt_sts $slot)
  echo "$slot" >> $LOG
  if [ $val = 0 ] || [ $val = 4 ]; then
    echo "2OU:" >> $LOG
    init_vishay_ic $slot $REXP_INTF 
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
