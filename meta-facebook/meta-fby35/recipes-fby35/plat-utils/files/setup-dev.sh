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

# shellcheck source=meta-facebook/meta-fby35/recipes-fby35/plat-utils/files/ast-functions
. /usr/local/fbpackages/utils/ast-functions

LTC4282_ADDR="44"
ADM1272_ADDR="1f"
LTC4282_CONTROL_REG="00"

function create_new_dev() {
  local dev_name=$1
  local addr=$2
  local bus=$3

  echo "$dev_name $addr" > /sys/class/i2c-dev/i2c-"${bus}"/device/new_device
}

function init_class1_dev() {
  #create the device of the inlet/outlet temp.
  create_new_dev "lm75" 0x4e 12
  create_new_dev "lm75" 0x4f 12

  #create the device of bmc/bb fru.
  create_new_dev "24c128" 0x51 11
  create_new_dev "24c128" 0x54 11

  local medusa_addr=""
  local chip=""
  local load_driver=false

  # Get register from different address to determine HSC chip
  if [ "$(/usr/sbin/i2cget -y 11 0x$LTC4282_ADDR 0x1 2> /dev/null | wc -l)" -eq "1" ]; then
    curr_val="$(/usr/sbin/i2cget -y 11 0x"$LTC4282_ADDR" 0x"$LTC4282_CONTROL_REG")"
    set_oc_auto_retry=$((curr_val | (0x01 << 2)))
    /usr/sbin/i2cset -y 11 0x"$LTC4282_ADDR" 0x"$LTC4282_CONTROL_REG" "$(printf '0x%x' $set_oc_auto_retry)"
    chip="ltc4282"
    medusa_addr=0x"$LTC4282_ADDR"
    load_driver=true
  elif [ "$(/usr/sbin/i2cget -y 11 0x"$ADM1272_ADDR" 0x1 2> /dev/null | wc -l)" -eq "1" ]; then
    chip="adm1272"
    medusa_addr=0x"$ADM1272_ADDR"
    load_driver=true
  else
    chip="ltc4287"
  fi
  if [ "$load_driver" = true ]; then
    create_new_dev $chip $medusa_addr 11
  fi

  # /mnt/data/kv_store checking, and create one if not exist
  if [ ! -d /mnt/data/kv_store ]; then
    /bin/mkdir -p /mnt/data/kv_store
  fi
  kv set bb_hsc_conf $chip persistent

  # check nic power to see if it's need to be set to standby mode
  set_nic_power
}

function init_class2_dev() {
  #create the device of the outlet temp.
  create_new_dev "lm75" 0x4f 2
  create_new_dev "lm75" 0x4f 12

  #create the device of bmc/nic fru.
  create_new_dev "24c128" 0x51 11
  create_new_dev "24c128" 0x54 11
}

function init_exp_dev() {
  for i in {1..4}; do
    bmc_location=$(get_bmc_board_id)
    if [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then 
      if [ "$(is_server_prsnt $i)" == "0" ]; then
        continue
      fi
      enable_server_i2c_bus $i
    fi    
    cpld_bus=$(get_cpld_bus $i)
    type_2ou=$(get_2ou_board_type "$cpld_bus")
    # check DPv2 x8 present
    prsnt_x8=$((type_2ou & 0x7))
    if [ "$prsnt_x8" -eq "7" ]; then
      create_new_dev "24c128" 0x51 "$cpld_bus"
    fi
  done
}

echo "Setup devs for fby35..."

#create the device of mezz card
create_new_dev "tmp421" 0x1f 8
create_new_dev "24c32" 0x50 8

bmc_location=$(get_bmc_board_id)
if [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
  #The BMC of class1
  init_class1_dev
elif [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
  #The BMC of class2
  init_class2_dev
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

init_exp_dev
set_vf_gpio

echo "Done."
