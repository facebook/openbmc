#!/bin/bash
#
# Copyright 2020-present Delta Eletronics, Inc. All Rights Reserved.
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

KERNEL_VERSION=$(uname -r)
start_of_mux_bus=14

# get from swpld1
get_board_type() {
    reg=$(i2cget -y -f 7 0x31 0x02)
    reg=$((reg >> 4))
    if [ $reg -eq 0 ]; then
        echo 0
        return
    elif [ $reg -eq 1 ]; then
        echo 1
        return
    else
        echo -1
    fi
}

get_board_rev() {
    reg=$(i2cget -y -f 7 0x31 0x02)
    reg=$((reg & 0x3))
    if [ $reg -eq 0 ]; then
        echo 0
        return
    elif [ $reg -eq 1 ]; then
        echo 1
        return
    else
        echo -1
    fi
}

get_mux_bus_num() {
    echo $((start_of_mux_bus + $1))
}

brd_type=$(get_board_type)
brd_rev=$(get_board_rev)

# Bus 0
i2c_device_add 0 0x50 24c02                 # PSU1 EEPROM
i2c_device_add 0 0x58 dps_driver            # PSU1 PSU_DRIVER

# Bus 1
i2c_device_add 1 0x50 24c02                 # PSU2 EEPROM
i2c_device_add 1 0x58 dps_driver            # PSU2 PSU_DRIVER

# Bus 2
# i2c_device_add 2 0x75 pca9548          # PCA9548

# Bus 3
i2c_device_add 3 0x4c emc2305          #Fan Controller
i2c_device_add 3 0x2d emc2305          #Fan Controller
i2c_device_add 3 0x2e emc2305          #Fan Controller
# i2c_device_add 3 0x4f tmp75            #Fan BD

# Bus 4
# i2c_device_add 4 0x4b tmp75            #Thermal RF
# i2c_device_add 4 0x49 tmp75            #Thermal LF
# i2c_device_add 4 0x4a tmp75            #Thermal Upper
# i2c_device_add 4 0x4e tmp75            #Thermal Lower

# Bus 5
# i2c_device_add 5 0x65 tps53647                   #VCC_VDD_0V8
# i2c_device_add 5 0x10 ir35223                    #VCC_AVDD_0V9
# i2c_device_add 5 0x64 tps53667                   #VCC_3V3

# Bus 6
i2c_device_add 6 0x50 24c08                        #SMB EEPROM

# Bus 7
i2c_device_add 7 0x32 swpld_1           #swpld1
i2c_device_add 7 0x34 swpld_2           #swpld2
i2c_device_add 7 0x35 swpld_3           #swpld3

# i2c-mux 2-0075: child bus 14-21

# i2c-mux 2, channel 1
i2c_device_add 14 0x50 24c64     #FAN1 Tray
# i2c-mux 2, channel 2
i2c_device_add 15 0x50 24c64     #FAN2 Tray
# i2c-mux 2, channel 3
i2c_device_add 16 0x50 24c64     #FAN3 Tray
# i2c-mux 2, channel 4
i2c_device_add 17 0x50 24c64     #FAN4 Tray
# i2c-mux 2, channel 5
i2c_device_add 18 0x50 24c64     #FAN5 Tray
# i2c-mux 2, channel 6
i2c_device_add 19 0x50 24c64     #FAN6 Tray
# i2c-mux 2, channel 7
i2c_device_add 20 0x27 pca9555   #PCA9555


#
# Check if I2C devices are bound to drivers. A summary message (total #
# of devices and # of devices without drivers) will be dumped at the end
# of this function.
#
i2c_check_driver_binding
