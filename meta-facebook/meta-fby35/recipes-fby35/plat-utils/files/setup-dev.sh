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
LTC4287_ADDR="11"
LTC4282_CONTROL_REG="00"
LTC4287_CONTROL_REG="47"
MEDUSA_HSC_BUS="11"
LTC2992_ADDR="6e"
INA238_ADDR_PSU="44"
INA238_ADDR_GND="41"
MEDUSA_48V_IO_EXP_ADDR="49"

function init_48V_medusa() {
  local medusa_adc_addr=("$LTC2992_ADDR" "$INA238_ADDR_PSU" "$INA238_ADDR_GND")
  local medusa_adc_devs=("ltc2992" "ina238" "ina238")
  local medusa_dev_addr=""
  local chip=""

  for i in "${!medusa_adc_addr[@]}"; do
    if /usr/sbin/i2cget -f -y "$MEDUSA_HSC_BUS" 0x"${medusa_adc_addr["$i"]}" 0 >/dev/null 2>&1; then
      medusa_dev_addr="$MEDUSA_HSC_BUS"-00"${medusa_adc_addr["$i"]}"
      if [ ! -L "${SYSFS_I2C_DEVICES}/$medusa_dev_addr/driver" ]; then
        i2c_device_add "$MEDUSA_HSC_BUS" 0x"${medusa_adc_addr["$i"]}" "${medusa_adc_devs["$i"]}"
        chip="${medusa_adc_devs["$i"]}"
      fi
      break
    fi
  done

  kv set medusa_adc_chip "$chip"
}

function init_class1_dev() {
  #create the device of the inlet/outlet temp.
  i2c_device_add 12 0x4e lm75
  i2c_device_add 12 0x4f lm75

  #create the device of bmc/bb fru.
  i2c_device_add 11 0x51 24c128
  i2c_device_add 11 0x54 24c128

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
    curr_val="$(/usr/sbin/i2cget -y 11 0x"$LTC4287_ADDR" 0x"$LTC4287_CONTROL_REG")"
    set_oc_auto_retry=$((curr_val | (0x07 << 3)))
    /usr/sbin/i2cset -y 11 0x"$LTC4287_ADDR" 0x"$LTC4287_CONTROL_REG" "$(printf '0x%x' $set_oc_auto_retry)"
    chip="ltc4287"
    medusa_addr=0x"$LTC4287_ADDR"
    load_driver=true
  fi
  if [ "$load_driver" = true ]; then
    i2c_device_add 11 $medusa_addr $chip
  fi

  kv set bb_hsc_conf $chip persistent

  # check nic power to see if it's need to be set to standby mode
  set_nic_power
}

function init_class2_dev() {
  #create the device of the outlet temp.
  i2c_device_add 2 0x4f lm75
  i2c_device_add 12 0x4f lm75

  #create the device of bmc/nic fru.
  i2c_device_add 11 0x51 24c128
  i2c_device_add 11 0x54 24c128
}

function init_exp_dev() {
  for i in {1..4}; do
    bmc_location=$(get_bmc_board_id)
    if [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
      if [ "$(is_server_prsnt "$i")" == "0" ]; then
        continue
      fi
      enable_server_i2c_bus "$i"
    fi
    cpld_bus=$(get_cpld_bus "$i")
    type_2ou=$(get_2ou_board_type "$cpld_bus")
    # check DPv2 x8 present
    prsnt_x8=$((type_2ou & 0x7))
    if [ "$prsnt_x8" -eq "7" ]; then
      i2c_device_add "$cpld_bus" 0x51 24c128
    fi
    # EEPROM on PRoT module
    if [ "$(is_prot_prsnt "$i")" == "1" ]; then
      i2c_device_add "$cpld_bus" 0x50 24c32
    fi
    # IO-exp on VF2
    set_vf_gpio "$i"
  done
}

echo "Setup devs for fby35..."

#create the device of mezz card
i2c_device_add 8 0x1f tmp421
i2c_device_add 8 0x50 24c32

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

# Check if the Medusa IO expender exist
if i2cget -f -y "$MEDUSA_HSC_BUS" 0x"$MEDUSA_48V_IO_EXP_ADDR" 0 >/dev/null 2>&1; then
  init_48V_medusa
fi

init_exp_dev

echo "Done."
