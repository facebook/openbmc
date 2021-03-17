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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh

# First, we will Enable write in WRITE_PROTECT(0x10) reg before adding the i2c
# pmbus device for dps1900. This will prevent some sysfs node from disappearing
# when we powercycle
i2c_dsp1900_create(){
  if ! i2cset -f -y "$1" 0x58 0x10 0; then
    echo "Fail to clear PSU pmbus WRITE_PROTECT(0x10) register"
  fi
  i2c_device_add "$1" 0x58 pmbus
}

# if applicable, saved the debug log for ir35223
ir35223_dbg_log(){
  log_data=("${!1}")
  if [ ${#log_data[@]} != 0 ]; then
    touch /var/log/failed_ir35223_log.txt
    for cmd in "${log_data[@]}"
    do
      echo "${cmd}" >> /var/log/failed_ir35223_log.txt;
    done
  fi
}

# There are some registers that need to be initialized in IR35223
# prior to loading PMBus module for yamp devices
i2c_ir35223_init(){
  # Array to hold failed data
  declare -a failed_ir35223_cmds

  # Step 1:0x005C [15:0] = 0x0102
  if ! i2cset -y 3 0x12 0xff 0x00; then
    failed_ir35223_cmds+=("Running i2cset -y 3 0x12 0xff 0x00 failed")
  fi
  if ! i2cset -y 3 0x12 0x5c 0x0102 w; then 
    failed_ir35223_cmds+=("Running i2cset -y 3 0x12 0x5c 0x0102 w failed")
  fi

  # Step 2: 0x043A [7:4] = 0xD
  if ! i2cset -y 3 0x12 0xff 0x04; then
    echo "Running i2cset -y 3 0x12 0xff 0x04 failed"
  fi
  if ! i2cset -y -m 0xf0 3 0x12 0x3a 0xd0; then 
    echo "Running i2cset -y -m 0xf0 3 0x12 0x3a 0xd0 failed"
  fi

  # Step 3: 0x0040 [7:0] = 0xFF
  if ! i2cset -y 3 0x12 0xff 0x00; then
    echo "Running i2cset -y 3 0x12 0xff 0x00 failed"
  fi
  if ! i2cset -y 3 0x12 0x40 0xff; then
    echo "Running i2cset -y 3 0x12 0x40 0xff failed"
  fi

  # Step 4: 0x0428 [3:0] = 0x2
  if ! i2cset -y 3 0x12 0xff 0x04; then
    echo "Running i2cset -y 3 0x12 0xff 0x04 failed"
  fi
  if ! i2cset -y -m 0x0f 3 0x12 0x28 0x02; then
    echo "Running i2cset -y -m 0x0f 3 0x12 0x28 0x02"
  fi

  # Step 5: 0x042E [5:0] = 0x0D
  if ! i2cset -y 3 0x12 0xff 0x04; then
    echo "Running i2cset -y 3 0x12 0xff 0x04"
  fi
  if ! i2cset -y -m 0x1f 3 0x12 0x2e 0x0d; then
    echo "i2cset -y -m 0x1f 3 0x12 0x2e 0x0d failed"
  fi

  # Step 6: 0x0444 [7:7] = 0x1
  if ! i2cset -y 3 0x12 0xff 0x04; then
    echo "Running i2cset -y -m 0x1f 3 0x12 0x2e 0x0d failed"
  fi
  if ! i2cset -y -m 0x80 3 0x12 0x44 0x80; then
    echo "Running i2cset -y -m 0x80 3 0x12 0x44 0x80 failed"
  fi

  # Step 7: Set Page back to 0
  if ! i2cset -y 3 0x12 0xff 0x00; then
    echo "i2cset -y 3 0x12 0xff 0x00"
  fi

  # Saved debug log
  ir35223_dbg_log failed_ir35223_cmds[@]
}

# First, take TPM out of reset, so that we can probe it later
# Active low - 1 means out of reset, 0 means in reset
gpio_set TPM_RST_N 1

# Probe the devices used by fscd first, just in case probing 
# taking too long, when one of modules are not in good shape.
# 1. Probe SUP Inlet1/2 sensor, used for fscd 
i2c_device_add 11 0x4c max6658
# 2. Probe TH3 temp sensor, also used for fscd
i2c_device_add 4 0x4d max6581

# Bus  0 - SMBus 1
i2c_device_add 0 0x20 tpm_i2c_infineon

# Bus  1 - SMBus 2
i2c_device_add 1 0x50 supsfp

# Bus  3 - SMBus 4 - SCB ECB/VRD and sensors
i2c_device_add 3 0x40 pmbus
i2c_device_add 3 0x41 pmbus
i2c_device_add 3 0x4e ucd90120
i2c_ir35223_init
i2c_device_add 3 0x42 pmbus
i2c_device_add 3 0x44 pmbus

# Bus  4 - SMBus 5 SCD
i2c_device_add 4 0x23 scdcpld
i2c_device_add 4 0x50 24c512

#
# Bus  2 - SMBus 3 - LC, MUXED as Bus 14-21
#
# Note:
#  1) 2-0075 may not be created if yamp is running an older kernel, so
#     let's check and create the device if needed.
#  2) if 2-0075 is already created (in device tree) but driver binding
#     failed, then we will need to bind the driver manually. The failure
#     usually happens when the chassis is power cycled, because 2-0075
#     cannot be detected until it's taken out of reset (by writing scdcpld
#     register LC_SMB_MUX_RST).
#
echo 0 > "$LC_SMB_MUX_RST"  # take the mux (2-0075) out of reset

SMB_MUX_DEV="2-0075"
SMB_MUX_PATH=$(i2c_device_sysfs_abspath "$SMB_MUX_DEV")
if [ ! -d "$SMB_MUX_PATH" ]; then
    i2c_mux_add_sync 2 0x75 pca9548 8
elif [ ! -L "${SMB_MUX_PATH}/driver" ]; then
    mux_driver="pca954x"
    echo "Manually bind $mux_driver to $SMB_MUX_DEV.."
    i2c_bind_driver "$mux_driver" "$SMB_MUX_DEV"

    # Wait till all the 8 channels (0-7) are populated; otherwise devices
    # behind the switch cannot be created.
    MUX_RETRY=0
    until [ -d "${SMB_MUX_PATH}/channel-7" ]; do
        usleep 10000 # sleep for 10 milliseconds
        MUX_RETRY=$((MUX_RETRY + 1))
        if [ "$MUX_RETRY" -gt 3 ]; then
            break
        fi
    done
fi

# Bus  5 - SMBus 6 PSU1
i2c_dsp1900_create 5

# For now, we assume that PSU2 and PSU3 are not present
#i2c_device_add 6 0x58 pmbus
#i2c_device_add 7 0x58 pmbus

# Bus  8 - SMBus 9 PSU4
i2c_dsp1900_create 8

# Bus  9 - SMBus 10 SUP DPM
#i2c_device_add 9 0x11 pmbus

# Bus  10 - SMBus 11 SUP POWER
i2c_device_add 10 0x21 cpupwr
i2c_device_add 10 0x27 mempwr

# Bus  11 - SMBus 12 SUP TEMP
i2c_device_add 11 0x40 pmbus

# Bus  12 - SMBus 13 SUP CPLD
i2c_device_add 12 0x43 supcpld

# Bus  13 - SMBus 14 FAN CTRL
i2c_device_add 13 0x4c max6658
i2c_device_add 13 0x52 24c512
i2c_device_add 13 0x60 fancpld

# Bus  14 - Muxed LC1
i2c_device_add 14 0x4e ucd90120
i2c_device_add 14 0x42 pmbus
i2c_device_add 14 0x20 pca9555
i2c_device_add 14 0x4c max6658
i2c_device_add 14 0x50 24c512

# Bus  15 - Muxed LC2
i2c_device_add 15 0x4e ucd90120
i2c_device_add 15 0x42 pmbus
i2c_device_add 15 0x20 pca9555
i2c_device_add 15 0x4c max6658
i2c_device_add 15 0x50 24c512

# Bus  16 - Muxed LC3
i2c_device_add 16 0x4e ucd90120
i2c_device_add 16 0x42 pmbus
i2c_device_add 16 0x20 pca9555
i2c_device_add 16 0x4c max6658
i2c_device_add 16 0x50 24c512

# Bus  17 - Muxed LC4
i2c_device_add 17 0x4e ucd90120
i2c_device_add 17 0x42 pmbus
i2c_device_add 17 0x20 pca9555
i2c_device_add 17 0x4c max6658
i2c_device_add 17 0x50 24c512

# Bus  18 - Muxed LC5
i2c_device_add 18 0x4e ucd90120
i2c_device_add 18 0x42 pmbus
i2c_device_add 18 0x20 pca9555
i2c_device_add 18 0x4c max6658
i2c_device_add 18 0x50 24c512

# Bus  19 - Muxed LC6
i2c_device_add 19 0x4e ucd90120
i2c_device_add 19 0x42 pmbus
i2c_device_add 19 0x20 pca9555
i2c_device_add 19 0x4c max6658
i2c_device_add 19 0x50 24c512

# Bus  20 - Muxed LC7
i2c_device_add 20 0x4e ucd90120
i2c_device_add 20 0x42 pmbus
i2c_device_add 20 0x20 pca9555
i2c_device_add 20 0x4c max6658
i2c_device_add 20 0x50 24c512

# Bus  21 - Muxed LC8
i2c_device_add 21 0x4e ucd90120
i2c_device_add 21 0x42 pmbus
i2c_device_add 21 0x20 pca9555
i2c_device_add 21 0x4c max6658
i2c_device_add 21 0x50 24c512
