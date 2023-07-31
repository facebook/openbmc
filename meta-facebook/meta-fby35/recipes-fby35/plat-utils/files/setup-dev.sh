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

MEDUSA_HSC_BUS="11"
LTC4282_ADDR="0x44"
ADM1272_ADDR="0x1f"
LTC4287_ADDR="0x11"
LTC4282_CONTROL_REG="0x00"
LTC4282_ADJUST_REG="0x11"
LTC4287_CONTROL_REG="0x47"
MEDUSA_48V_IO_EXP_ADDR="0x49"
INA238_ADDR_PSU="44"
INA238_ADDR_GND="41"
ISL28022_ADDR_PSU="47"
ISL28022_ADDR_GND="43"
MAXIM_SOLUTION="0"
MPS_SOLUTION="1"

function read_dev() {
  local bus=$1
  local addr=$2
  local reg=$3
  local val

  for _ in {1..3}; do
    if val=$(/usr/sbin/i2cget -f -y "$bus" "$addr" "$reg" 2>/dev/null); then
      echo "$val"
      return 0
    fi
  done

  return 1
}

function init_48V_medusa() {
  local medusa_adc_addr=("$INA238_ADDR_PSU" "$INA238_ADDR_GND")
  local medusa_adc_devs=("ina238" "ina238")
  local medusa_dev_addr=""
  local chip=""

  echo "Init new 48V medusa..."

  for i in "${!medusa_adc_addr[@]}"; do
    if read_dev "$MEDUSA_HSC_BUS" 0x"${medusa_adc_addr["$i"]}" 0 >/dev/null; then
      medusa_dev_addr="$MEDUSA_HSC_BUS"-00"${medusa_adc_addr["$i"]}"
      if [ ! -L "${SYSFS_I2C_DEVICES}/$medusa_dev_addr/driver" ]; then
        chip="${medusa_adc_devs["$i"]}"
        i2c_device_add "$MEDUSA_HSC_BUS" 0x"${medusa_adc_addr["$i"]}" "${medusa_adc_devs["$i"]}"
      fi
    else
      chip="ISL28022"
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

  if read_dev $MEDUSA_HSC_BUS $LTC4287_ADDR 0x1 >/dev/null; then
    if curr_val=$(read_dev $MEDUSA_HSC_BUS $LTC4287_ADDR $LTC4287_CONTROL_REG); then
      set_oc_auto_retry=$((curr_val | (0x7 << 3)))
      /usr/sbin/i2cset -y $MEDUSA_HSC_BUS $LTC4287_ADDR $LTC4287_CONTROL_REG "$set_oc_auto_retry"
    fi
    chip="ltc4287"
    medusa_addr=$LTC4287_ADDR
    load_driver=true
  elif read_dev $MEDUSA_HSC_BUS $ADM1272_ADDR 0x1 >/dev/null; then
    chip="adm1272"
    medusa_addr=$ADM1272_ADDR
    load_driver=true
  elif read_dev $MEDUSA_HSC_BUS $LTC4282_ADDR 0 >/dev/null; then
    if curr_val=$(read_dev $MEDUSA_HSC_BUS $LTC4282_ADDR $LTC4282_CONTROL_REG); then
      set_oc_auto_retry=$((curr_val | (0x1 << 2)))
      /usr/sbin/i2cset -y $MEDUSA_HSC_BUS $LTC4282_ADDR $LTC4282_CONTROL_REG "$set_oc_auto_retry"
    fi
    chip="ltc4282"
    medusa_addr=$LTC4282_ADDR
    load_driver=true
  else
    load_driver=false
  fi

  if [ "$load_driver" = true ]; then
    i2c_device_add $MEDUSA_HSC_BUS $medusa_addr $chip
    if [ "$chip" == "ltc4282" ]; then
      if curr_val=$(read_dev $MEDUSA_HSC_BUS $LTC4282_ADDR $LTC4282_ADJUST_REG); then
        set_12bit_mode=$((curr_val & ~0x1))
        /usr/sbin/i2cset -f -y $MEDUSA_HSC_BUS $LTC4282_ADDR $LTC4282_ADJUST_REG "$set_12bit_mode"
      fi
    fi
  fi

  kv set medusa_hsc_conf $chip persistent

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

function init_adc_upper_bound() {
  #ADC0 ~ ADC7 upper bound value
  local adc0_upper_bound=("269" "265" "257" "2E1" "259" "31C" "212" "1BA")
  #ADC8 ~ ADC15 upper bound value
  local adc1_upper_bound=("109" " " "2A9" "32B" "265" "257" "262" " ")
  local efuse_type
  efuse_type=$(gpio_get_value P12V_EFUSE_DETECT_N)
  if [ "$efuse_type" -eq "$MPS_SOLUTION" ]; then
    adc1_upper_bound[2]="21F"
  fi
  for i in {0..7}; do
    if [[ "${adc0_upper_bound["$i"]}" != " " ]]; then
      echo $((0x"${adc0_upper_bound["$i"]}")) > /sys/bus/iio/devices/iio\:device0/events/in_voltage"$i"_thresh_rising_value
      echo 1 > /sys/bus/iio/devices/iio\:device0/events/in_voltage"$i"_thresh_rising_en
    fi
  done
  for i in {0..7}; do
    if [[ "${adc1_upper_bound["$i"]}" != " " ]]; then
      echo $((0x"${adc1_upper_bound["$i"]}")) > /sys/bus/iio/devices/iio\:device1/events/in_voltage"$i"_thresh_rising_value
      echo 1 > /sys/bus/iio/devices/iio\:device1/events/in_voltage"$i"_thresh_rising_en
    fi
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

# Check if the Medusa IO expender or ADC chip exist (To prevent missing init due to any of them is broken)
if read_dev $MEDUSA_HSC_BUS $MEDUSA_48V_IO_EXP_ADDR 0 >/dev/null; then
  init_48V_medusa
elif read_dev $MEDUSA_HSC_BUS 0x$INA238_ADDR_PSU 0 >/dev/null; then
  init_48V_medusa
elif read_dev $MEDUSA_HSC_BUS 0x$INA238_ADDR_GND 0 >/dev/null; then
  init_48V_medusa
elif read_dev $MEDUSA_HSC_BUS 0x$ISL28022_ADDR_PSU 0 >/dev/null; then
  init_48V_medusa
elif read_dev $MEDUSA_HSC_BUS 0x$ISL28022_ADDR_GND 0 >/dev/null; then
  init_48V_medusa
else
  echo "Skip new 48V medusa init..."
fi

init_adc_upper_bound
init_exp_dev

echo "Done."
