#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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

# The UCD OPERATION register may get corrupted if clock stretch happens
# when accessing the device. Once the register write is corrupted,
# then the subsequent voltage readings will also be incorrect.
# To recover from such scenario, reset the OPERATION register back to default.
pmbus_reset_OPERATION_register() {
  page_count=$1
  address=$2
  bus=$3

  for (( page=0; page<page_count; page++ )) ;do
    #select the page (page code : 0 )
    i2cset -y -f "$bus" "$address" 0 "$page"
    #reset the operation to default(operation code: 1)
    i2cset -y -f "$bus" "$address" 1 0
  done
}

# unbind and bind the i2c driver
i2c_driver_rebind() {
  device=$1
  driver_sysfs=$2

  echo  "$device" > "$driver_sysfs"/unbind
  sleep 1
  echo  "$device" > "$driver_sysfs"/bind
  sleep 1
}

# During bootup validate whether the hwmon sysfs exists and
# Input voltage value within threshold range. If not expected
# unbind and bind the driver.
hwmon_sanitize_input_voltage() {
  page_count=$1
  bus=$2
  address=$3

  device="${bus}"-00"${address:2}"
  hwnon_sysfs=( /sys/bus/i2c/devices/"${device}"/hwmon/hwmon* )
  driver_sysfs=$(readlink -f /sys/bus/i2c/devices/"${device}"/driver)

  for retry in {1..5} ; do
    error=0
    #Reset the OPERATION code for UCD sensor
    if  [[ ${driver_sysfs} =~ "ucd9000" ]]; then
      pmbus_reset_OPERATION_register "$page_count" "${address}" "$bus"
    fi
    #Verify sensor value
    for page in $(seq 1 "$page_count") ; do
      in_file="${hwnon_sysfs[*]}"/in"$page"_input
      if [ ! -f "${in_file}" ] ;then
        error=1; break
      fi
      val=$(cat "${in_file}")
      if [ -z "$val" ] || [[ $val -gt 20000 ]];then
        error=1; break
      fi
    done

    if [ $error -eq 1 ] ;then
      echo "Sensor addr=$address init failed retry=$retry bind/unbind device"
      i2c_driver_rebind "$device" "$driver_sysfs"
    else
      break
    fi
  done
}


# Return pmbus device info, such as bus number,
# device address and sensor page count.
pmbus_get_device_info() {
  smb_mp2978_1=(1 0x53 2) # (<bus no> <addr> <page count>)
  smb_mp2978_2=(1 0x59 2) # (<bus no> <addr> <page count>)
  i2c_ucd_dev=$1

  if [ "$i2c_ucd_dev" == "smb_pwrseq_1" ] || [ "$i2c_ucd_dev" == "smb_pwrseq_2" ] ;then
    address=$(/usr/bin/kv get "${i2c_ucd_dev}"_addr)
    page_count=$(/usr/bin/kv get "${i2c_ucd_dev}"_page_count)
    bus=5
  elif [ "$i2c_ucd_dev" == "smb_mp2978_1" ] ;then
    address=${smb_mp2978_1[1]}
    page_count=${smb_mp2978_1[2]}
    bus=${smb_mp2978_1[0]}
  else
    address=${smb_mp2978_2[1]}
    page_count=${smb_mp2978_2[2]}
    bus=${smb_mp2978_2[0]}
  fi

  echo "$address" "$bus" "$page_count"
}

# Workaround for the following issue occurred in UCD/MP sensor
# Scenario 1: some sensors could not be enumerated
# Scenario 2: driver initialization ran into incorrect co-efficient which results in high values
hwmon_fix_driver_binding () {
  for i2c_dev in "$@" ; do
    read -r address bus page_count < <(pmbus_get_device_info "$i2c_dev")

    if [ -z "$address" ] || [ -z "$page_count" ] ;then
      echo "UCD device info not found"
    else
      hwmon_sanitize_input_voltage "$page_count" "$bus" "${address}"
    fi
  done
}

