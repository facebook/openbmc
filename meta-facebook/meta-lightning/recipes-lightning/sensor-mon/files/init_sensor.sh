#!/bin/sh
#
# Copyright 2016-present Facebook. All Rights Reserved.
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

modprobe lm75
modprobe pmbus

# Enable the ADC inputs;  adc0 - adc7 are connected to various voltage sensors

echo 1 > /sys/devices/platform/ast_adc.0/adc0_en
echo 1 > /sys/devices/platform/ast_adc.0/adc1_en
echo 1 > /sys/devices/platform/ast_adc.0/adc2_en
echo 1 > /sys/devices/platform/ast_adc.0/adc3_en
echo 1 > /sys/devices/platform/ast_adc.0/adc4_en
echo 1 > /sys/devices/platform/ast_adc.0/adc5_en
echo 1 > /sys/devices/platform/ast_adc.0/adc6_en
echo 1 > /sys/devices/platform/ast_adc.0/adc7_en


I2C_BUS_FCB=5

I2C_BUS_PEB=4

# NCT7904 is the fan controller on the FCB board
I2C_ADDR_FCB_NCT7904=0x2d

# ADM1276 is the HSC on FCB board
I2C_ADDR_FCB_HSC=0x22

# PCA9551 is the LED controller on the FCB board
I2C_ADDR_FCB_LED=0x60

# ADM1278 is the HSC on PEB board
I2C_ADDR_PEB_HSC=0x11

# Enable the Pins to sense the Temperature VSEN23_MD and VSEN45_MD
i2cset -y $I2C_BUS_FCB $I2C_ADDR_FCB_NCT7904 0x2e 0x5 b

# Enable the Power IN averaging setting
i2cset -y $I2C_BUS_FCB $I2C_ADDR_FCB_HSC 0xd4 0xaf b

# Light on FCB LED(turn blue)
i2cset -y $I2C_BUS_FCB $I2C_ADDR_FCB_LED 0x05 0x00  #LED 0~3
i2cset -y $I2C_BUS_FCB $I2C_ADDR_FCB_LED 0x06 0x00  #LED 4~5

# Initialize the 7904 monitor_flag register
i2cset -y $I2C_BUS_FCB $I2C_ADDR_FCB_NCT7904 0xba 0x00

# Enable the Power IN averaging setting
i2cset -y $I2C_BUS_PEB $I2C_ADDR_PEB_HSC 0xd4 0x3f1c w

# Enable the PEB HSC
i2cset -y $I2C_BUS_PEB $I2C_ADDR_PEB_HSC 0xd3 0x01 w

#Enable the ADC controller for ADC0~ADC7
devmem 0x1e6e9000 32 0x00ff000f
