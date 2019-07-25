#!/bin/sh
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

# First, take TPM out of reset, so that we can probe it later
# Active low - 1 means out of reset, 0 means in reset
gpio_set TPM_RST_N 1

# Bus  2 - SMBus 3 - LC, MUXED as Bus 14-21
# Create i2c switch 2-0075 while provides 8 child buses 14 - 21.
# This step is only needed in kernel 4.1: the device will be created in
# device tree in new kernel versions.
if uname -r | grep "4.1.*"; then
    i2c_device_add 2 0x75 pca9548
fi

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
i2c_device_add 3 0x42 pmbus
i2c_device_add 3 0x44 pmbus

# Bus  4 - SMBus 5 SCD
i2c_device_add 4 0x23 scdcpld
i2c_device_add 4 0x50 24c512
# Unreset LC SMBUS MUX on the Switch Card
echo 0 > $LC_SMB_MUX_RST

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
