#!/bin/bash
#
# Copyright 2018-present Facebook. All Rights Reserved.
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
# This script will wake up every 10 seconds and do the following
#    1. Try to discover any new PIM(s) plugged in
#    2. For all discovered PIMs, attempt to turn on the PIM and
#       the gearbox chip on it
#

. /usr/local/bin/openbmc-utils.sh

pim_list=$(seq 0 7)
pim_found=(0 0 0 0 0 0 0 0)
pim_bus=(14 15 16 17 18 19 20 21)
pca_addr=20

create_pim_gpio() {
  local base lc
  lc=$1
  chip=$2

  if [ -e LC${lc}_STATUS_GREEN_L ]; then
      # already created GPIOs for such pim
      return
  fi

  gpio_export_by_offset ${chip} 1 LC${lc}_DPM_POWER_UP
  gpio_export_by_offset ${chip} 4 LC${lc}_SCD_RESET_L
  gpio_export_by_offset ${chip} 5 LC${lc}_SCD_CONFIG_L
  gpio_export_by_offset ${chip} 6 LC${lc}_SATELLITE_PROG
  gpio_export_by_offset ${chip} 7 LC${lc}_LC_PWR_CYC
  gpio_export_by_offset ${chip} 8 LC${lc}_BAB_SYS_RESET_L
  gpio_export_by_offset ${chip} $((8 + 5)) LC${lc}_FAST_JTAG_EN
  gpio_export_by_offset ${chip} $((8 + 6)) LC${lc}_STATUS_RED_L
  gpio_export_by_offset ${chip} $((8 + 7)) LC${lc}_STATUS_GREEN_L
  logger pim_enable: registered PIM${lc} with GPIO chip ${chip}
}

power_on_pim() {
  local lc old ver
  lc=$1
  old=$(gpio_get LC${lc}_DPM_POWER_UP keepdirection)
  if [ $old -eq 1 ]; then
      # already powered on, skip
      return
  fi
  gpio_set LC${lc}_DPM_POWER_UP 1 # all power on
  sleep 1
  gpio_set LC${lc}_SCD_RESET_L 1     # scd out of reset
  gpio_set LC${lc}_SCD_CONFIG_L 1    # scd not in config
  gpio_set LC${lc}_BAB_SYS_RESET_L 1 # gearbox out of reset
  # If linecard FPGA version is before v6, need to set SATELLITE_PROG to 0.
  # Otherwise, need to set it to 1.
  # However, the reading of the linecard FPGA version is not reliable,
  # we will set SATELLITE_PROG to 1 at all time, assuming the linecard FPGA
  # is upgraded to the latest.
  gpio_set LC${lc}_SATELLITE_PROG 1
  gpio_set LC${lc}_STATUS_RED_L 1 # turn off red
  gpio_set LC${lc}_STATUS_GREEN_L 0 # turn on green
  logger pim_enbale: powered on PIM${lc}
}

#clear pimserial endpoint cache if pim doesn't exist but the pimserial file does.
clear_pimserial_cache(){
  for i in "${pim_list[@]}"
  do
    pimserial_number="${1}"
    pimserial_path="$SCDCPLD_SYSFS_DIR/lc${pimserial_number}_prsnt_sta"
    pimserial_present="$(cat ${pimserial_path} 2> /dev/null | head -n 1)"
    pimserial_cache_path=/tmp/pim${pimserial_number}_serial.txt
    
    if [ "${pimserial_present}" == 0x0 ] && [ -f "${pimserial_cache_path}" ]; then
      #pim device doesn't exist but pimserial cache txt file exist, so remove it.
      rm -rf "${pimserial_cache_path}"
    fi
  done
}

while true; do
  # 1. For any uncovered PIM, try to discover
  for i in ${pim_list[@]}
  do
    if [ "${pim_found[$i]}" -eq "0" ]; then
      pim_number=$(expr $i + 1)
      pim_addr=${pim_bus[$i]}-00$pca_addr  # 14-0020 for example
      # Check if Switch card senses the PIM presence
      pim_path="$SCDCPLD_SYSFS_DIR/lc${pim_number}_prsnt_sta"
      pim_present=`cat $pim_path 2> /dev/null | head -n 1`
      # There is a HW bug which makes pim presence unreliable.
      #if [ "$pim_present" == "0x1" ]; then
      if true; then
         dev_path=/sys/bus/i2c/devices/$pim_addr
         drv_path=/sys/bus/i2c/drivers/pca953x/$pim_addr
         if [ ! -e $drv_path ]; then
           # Driver doesn't exist
           if [ -e $dev_path ]; then
              # Device exists. Meaning previous probe failed. Clean up previous failure first
              i2c_device_delete ${pim_bus[$i]} 0x$pca_addr
              # Give some time to kernel
              sleep 1
           fi
           # Probe this device
           reg0_val=`i2cget -y ${pim_bus[$i]} 0x$pca_addr 0 2>&1`
           if [[ ! "$reg0_val" == *"Error"* ]]; then
              i2c_device_add ${pim_bus[$i]} 0x$pca_addr pca9555
           fi
         fi
         # Check if device was probed and driver was installed
         if [ -e $drv_path/gpio ]; then
           create_pim_gpio $((i+1)) ${pim_addr}
           pim_found[$i]=1
         fi
      fi
    fi
  done
  # 2. For discovered gpios, try turning it on
  for i in ${pim_list[@]}
  do
    if [ "${pim_found[$i]}" -eq "1" ]; then
       power_on_pim $((i + 1))
    fi
    
    #clear pimserial endpoint cache
    clear_pimserial_cache $((i+1))


  done
  # Sleep 5 seconds until the next loop.
  # reduce to 5 seconds for faster scan.
  sleep 5
done
