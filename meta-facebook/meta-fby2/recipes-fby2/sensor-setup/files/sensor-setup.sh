#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
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
# Provides:          sensor-setup
# Required-Start:    power-on
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Power on micro-server
### END INIT INFO
. /usr/local/fbpackages/utils/ast-functions

# Eventually, this will be used to configure the various (mostly
# i2c-based) sensors, once we have a kernel version that supports
# doing this more dynamically.
#
# For now, we're using it to install the lm75 and pmbus module so that it
# can detect the fourth temperature sensor, which is located
# on the uServer, which doesn't get power until power-on executes.
#
# Similarly, the pmbus sensor seems to have an easier time of
# detecting the NCP4200 buck converters after poweron.  This has not
# been carefully explored.
kernel_ver=$(get_kernel_ver)
if [ $kernel_ver == 4 ]; then
    modprobe lm75
    modprobe pmbus
    
    # Enable the ADC inputs;  adc0 - adc15 are connected to various voltage sensors
    # adc12 - adc15 are for SLOT_TYPE
    
    PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
    
    # setup ADC channels
    
    ADC_PATH="/sys/devices/platform/ast_adc.0"
    # For FBY2 tyep V:
    # channel 0:  r1:  5.36K; r2:  2.0K; v2: 0mv
    # channel 1:  r1:  15.8K; r2:  2.0K; v2: 0mv
    # channel 2:  r1:  2.87K; r2:  2.0K; v2: 0mv
    # channel 3:  r1:  15.8K; r2:  2.0K; v2: 0mv
    # channel 4:  r1:  15.8K; r2:  2.0K; v2: 0mv
    # channel 5:  r1:  15.8K; r2:  2.0K; v2: 0mv
    # channel 6:  r1:  15.8K; r2:  2.0K; v2: 0mv
    # channel 7:  r1:  2.87K; r2:  2.0K; v2: 0mv
    # channel 8:  r1:     1K; r2:  0.0K; v2: 0mv
    # channel 9:  r1:     1K; r2:  0.0K; v2: 0mv
    # channel 10: r1:  1.69K; r2:  2.0K; v2: 0mv
    # channel 11: r1:    665; r2:  2.0K; v2: 0mv
    # channel 12: r1:     1K; r2:    0K; v2: 0mv
    # channel 13: r1:     1K; r2:    0K; v2: 0mv
    # channel 14: r1:     1K; r2:    0K; v2: 0mv
    # channel 15: r1:     1K; r2:    0K; v2: 0mv
    
    config_adc() {
        channel=$1
        r1=$2
        r2=$3
        v2=$4
        echo $r1 > ${ADC_PATH}/adc${channel}_r1
        echo $r2 > ${ADC_PATH}/adc${channel}_r2
        echo $v2 > ${ADC_PATH}/adc${channel}_v2
        echo 1 > ${ADC_PATH}/adc${channel}_en
    }
    
    config_adc 0  5360  2000 0
    config_adc 1 15800  2000 0
    config_adc 2  2870  2000 0
    config_adc 3 15800  2000 0
    config_adc 4 15800  2000 0
    config_adc 5 15800  2000 0
    config_adc 6 15800  2000 0
    config_adc 7  2870  2000 0
    config_adc 8  	 0  1000 0
    config_adc 9  	 0  1000 0
    config_adc 10 1690  2000 0
    config_adc 11  665  2000 0
    config_adc 12    0  1000 0
    config_adc 13    0  1000 0
    config_adc 14    0  1000 0
    config_adc 15    0  1000 0
fi

# Barseboard Yosemite V2 & Yosemite V2.50
# Baseboard        Board_ID Rev_ID[2] Rev_ID[1] Rev_ID[0]
# Test board PoC       1       0         0         0
# Test board EVT       1       0         0         1
# YV2 PoC              0       0         0         0
# YV2 EVT              0       0         0         1
# YV2 DVT              0       0         1         0
# YV2 PVT              0       0         1         1
# YV2.50               1       1         X         X

# Enable GPIOY3: BoardId (Yosemite V2 or Yosemite V2.50)
devmem_clear_bit $(scu_addr a4) 11
devmem_clear_bit $(scu_addr 94) 11
gpio_export BOARD_ID GPIOY3

# Set up to read the board revision pins, GPIOY0, Y1, Y2
devmem_clear_scu70_bit 19
devmem_clear_bit $(scu_addr a4) 8
devmem_clear_bit $(scu_addr a4) 9
devmem_clear_bit $(scu_addr a4) 10

gpio_export BOARD_REV_ID0 GPIOY0
gpio_export BOARD_REV_ID1 GPIOY1
gpio_export BOARD_REV_ID2 GPIOY2

spb_type=$(get_spb_type)

# ADM1278 Configuration
if [ $spb_type == "1" ]; then
   # Yosemite V2.50 Platform
   # Clear PEAK_PIN & PEAK_IOUT register
   i2cset -y -f 10 0x40 0xd0 0x0000 w
   i2cset -y -f 10 0x40 0xda 0x0000 w

   #
   i2cset -y -f 10 0x40 0xd4 0x3f1c w
   i2cset -y -f 10 0x40 0xd5 0x0400 w
   i2cset -y -f 10 0x40 0xd8 0x0003 w

   # calibrtion to get HSC to trigger 60A based on EE team input
   # 0xe18 = 3608 (dec)
   #(3608*10-20475)/ (800*0.3)= 65.0208A
   # 65.0208* 0.92259 = 59.9875A
   i2cset -y -f 10 0x40 0x4a 0x0e18 w
elif [ $spb_type == "2" ]; then
   # FBND Platform
   # Clear PEAK_PIN & PEAK_IOUT register
   i2cset -y -f 10 0x40 0xd0 0x0000 w
   i2cset -y -f 10 0x40 0xda 0x0000 w

   #
   i2cset -y -f 10 0x40 0xd4 0x3d1c w
   i2cset -y -f 10 0x40 0xd5 0x0400 w

   # calibrtion to get HSC to trigger 63A based on EE team input
   # 0xe64 = 3686 (dec)
   #(3684*10-20475)/ (800*0.3)= 68.1875A
   # 68.1875* 0.92355 = 62.9746A
   i2cset -y -f 10 0x40 0x4a 0x0e64 w
else
   # Yosemite V2 Platform
   # Clear PEAK_PIN & PEAK_IOUT register
   i2cset -y -f 10 0x40 0xd0 0x0000 w
   i2cset -y -f 10 0x40 0xda 0x0000 w

   #
   i2cset -y -f 10 0x40 0xd4 0x3f1c w
   i2cset -y -f 10 0x40 0xd5 0x0400 w

   # calibrtion to get HSC to trigger 50A based on EE team input
   # 0xd14 = 3348 (dec)
   #(3348*10-20475)/ (800*0.3)= 54.1875A
   # 54.1875* 0.92190 = 49.9554A
   i2cset -y -f 10 0x40 0x4a 0x0d14 w
fi

if [ $kernel_ver == 4 ]; then
  rmmod adm1275
  modprobe adm1275
fi
