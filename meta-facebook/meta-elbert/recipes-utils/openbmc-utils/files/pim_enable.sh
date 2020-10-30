#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
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
# This script will wake up every 5 seconds and do the following
#    1. Try to discover any new PIM(s) plugged in
#    2. For all discovered PIMs, attempt to turn on the PIM and
#       the gearbox chip on it
#

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

pim_index=(0 1 2 3 4 5 6 7)
pim_found=(0 0 0 0 0 0 0 0)
pim_bus=(16 17 18 23 20 21 22 19)
dpm_addr=4e

try_create_gpio() {
    chip="$1"
    pin_id="$2"
    gpioname="$3"

    # Export GPIO if it doesn't exist
    gpio_num=$(gpio_name2value "$gpioname")
    if ! [[ "$gpio_num" =~ ^[0-9]+$ ]]; then
        gpio_export_by_offset "${chip}" "$pin_id" "$gpioname"
    fi
}

create_pim_gpio() {
  local pim
  pim=$1
  chip=$2

  fru="$(peutil "$pim" 2>&1)"
  if echo "$fru" | grep -q '88-16CD'; then
      pim_type='PIM16Q'
      try_create_gpio "$chip" 22 PIM"$pim"_FPGA_RESET_L  # GPIO17 PIN26
      try_create_gpio "$chip" 4  PIM"$pim"_FULL_POWER_EN # GPIO9  PIN14
      try_create_gpio "$chip" 20 PIM"$pim"_FPGA_DONE     # GPIO15 PIN24
      try_create_gpio "$chip" 21 PIM"$pim"_FPGA_INIT     # GPIO16 PIN25
  elif echo "$fru" | grep -q '88-8D'; then
      pim_type='PIM8DDM'
      try_create_gpio "$chip" 22 PIM"$pim"_FPGA_RESET_L  # GPIO17 PIN26
      try_create_gpio "$chip" 8  PIM"$pim"_FULL_POWER_EN # GPI1   PIN22
      try_create_gpio "$chip" 20 PIM"$pim"_FPGA_DONE     # GPIO15 PIN24
      try_create_gpio "$chip" 21 PIM"$pim"_FPGA_INIT     # GPIO16 PIN25
  else
      logger pim_enable: Could not identify PIM"${pim}" model
      return
  fi
  logger pim_enable: registered "${pim_type}" PIM"${pim}" with GPIO chip "${chip}"
}

# Clear pimserial endpoint cache if pim doesn't exist but the pimserial file does
clear_pimserial_cache() {
  pimserial_number="${1}"
  pimserial_path="$SMBCPLD_SYSFS_DIR/pim${pimserial_number}_present"
  pimserial_present=$(head -n 1 < "$pimserial_path")
  pimserial_cache_path=/tmp/pim${pimserial_number}_serial.txt

  if [ "${pimserial_present}" == 0x0 ] && [ -f "${pimserial_cache_path}" ]; then
    # Pim device doesn't exist but pimserial cache txt file exist, so remove it
    rm -rf "${pimserial_cache_path}"
  fi
}

while true; do
  # 1. For any uncovered PIM, try to discover
  for i in "${pim_index[@]}"
  do
    if [ "${pim_found[$i]}" -eq "0" ]; then
      pim_number="$((i+2))"
      pim_addr=${pim_bus[$i]}-00$dpm_addr  # 16-004e for example
      # Check if Switch card senses the PIM presence
      pim_path="$SMBCPLD_SYSFS_DIR/pim${pim_number}_present"
      pim_present="$(head -n 1 "$pim_path")"
      if [ "$pim_present" == "0x1" ]; then
         # Check if device was probed and driver was installed
         drv_path=/sys/bus/i2c/drivers/ucd9000/$pim_addr
         if [ -e "$drv_path"/gpio ]; then
           create_pim_gpio "$((i+2))" "${pim_addr}"
           pim_found[$i]=1
         fi
      fi
    fi
  done

  # 2. For discovered gpios, try turning it on
  for i in "${pim_index[@]}"
  do
    if [ "${pim_found[$i]}" -eq "1" ]; then
       if [ ! -f "/tmp/.pim$((i+2))_powered_off" ]; then
          power_on_pim $((i+2)) y
       else
          power_off_pim $((i+2)) y
       fi
    fi

    # Clear pimserial endpoint cache
    clear_pimserial_cache $((i+2))

  done
  # Sleep 5 seconds until the next loop.
  # reduce for faster scan.
  sleep 5
done
