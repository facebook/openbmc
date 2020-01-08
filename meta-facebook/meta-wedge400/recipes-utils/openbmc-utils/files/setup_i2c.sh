#!/bin/bash
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
#    i2c-mux 2-0070:  child bus 14-21
#    i2c-mux 8-0070:  child bus 22-29
#    i2c-mux 11-0070: child bus 30-37
    start_of_mux_bus=14
else
    start_of_mux_bus=16
fi

# get from smb cpld
get_board_type() {
    reg=$(i2cget -y -f 12 0x3e 0x00)
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
    reg=$(i2cget -y -f 12 0x3e 0x00)
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

# # Bus 2
i2c_device_add 2 0x3e scmcpld          # SCMCPLD

# # Bus 12
i2c_device_add 12 0x3e smb_syscpld     # SMB_SYSCPLD

# # i2c-mux 8, channel 7
i2c_device_add "$(get_mux_bus_num 15)" 0x3e smb_pwrcpld # SMB_PWRCPLD

# # Bus 13
i2c_device_add 13 0x60 domfpga        # DOM FPGA 1

# # Bus 4
i2c_device_add 4 0x27 pca9555         # PCA9555

# # Bus 5
i2c_device_add 5 0x60 domfpga         # DOM FPGA 2

# # Bus 6
i2c_device_add 6 0x51 24c64            # SMB EEPROM
i2c_device_add 6 0x21 pca9534          # PCA9534

# # Bus 9
i2c_device_add 9 0x74 si5391b          # SI5391B

# # Bus 1
i2c_device_add 1 0x3a powr1220          # SMB power sequencer
i2c_device_add 1 0x4d ir35215           # TH3 serdes voltage/current monitor on the left
i2c_device_add 1 0x47 ir35215           # TH3 serdes voltage/current monitor on the right
if [ "$brd_type" = "0" ]; then          # Only Wedge400
    i2c_device_add 1 0x60 isl68137      # TH3 core voltage/current monitor
elif [ "$brd_type" = "1" ]; then        # Only Wedge400-2
    i2c_device_add 1 0x40 xdpe132g5c    # Wedge400-2 GB core voltage/current monitor
    if [ "$brd_rev" = "1" ]; then
        i2c_device_add 1 0x0e pxe1211c  # Wedge400-2 EVT2 GB serdes voltage/current monitor
    else
        i2c_device_add 1 0x43 ir35215   # Wedge400-2 GB serdes voltage/current monitor
    fi
fi

# # Bus 3
i2c_device_add 3 0x48 tmp75            # SMB temp. sensor
i2c_device_add 3 0x49 tmp75            # SMB temp. sensor
i2c_device_add 3 0x4a tmp75            # SMB temp. sensor
i2c_device_add 3 0x4b tmp75            # SMB temp. sensor
i2c_device_add 3 0x4c tmp421           # SMB temp. sensor
i2c_device_add 3 0x4e tmp421           # SMB temp. sensor
if [ "$brd_type" = "0" ]; then         # Only Wedge400
i2c_device_add 3 0x4f tmp422           # TH3 temp. sensor
fi
if [ $((brd_type)) -eq 1 ]; then       # Only Wedge400C
i2c_device_add 3 0x2a net_asic        # GB temp. sensor
fi

# # i2c-mux 2, channel 1
i2c_device_add "$(get_mux_bus_num 0)" 0x10 adm1278 # SCM Hotswap

# # i2c-mux 2, channel 2
i2c_device_add "$(get_mux_bus_num 1)" 0x4c tmp75   # SCM temp. sensor
i2c_device_add "$(get_mux_bus_num 1)" 0x4d tmp75   # SCM temp. sensor

# # i2c-mux 2, channel 4
i2c_device_add "$(get_mux_bus_num 3)" 0x52 24c64   # EEPROM

# # i2c-mux 2, channel 5
i2c_device_add "$(get_mux_bus_num 4)" 0x50 24c02   # BMC54616S EEPROM

# # i2c-mux 2, channel 6
i2c_device_add "$(get_mux_bus_num 5)" 0x52 nvme    # NVME

# # i2c-mux 2, channel 8
i2c_device_add "$(get_mux_bus_num 7)" 0x6c si53108 # PCIE clock buffer

# # i2c-mux 8, channel 1
is_pem1=$(
    pem-util pem1 --get_pem_info | grep WEDGE400-PEM
    echo $?
)
# # ltc4282 only support registers 0~0x4f
if [ "$is_pem1" = "1" ]; then
    i2c_device_add "$(get_mux_bus_num 8)" 0x58 psu_driver   # PSU1 Driver
else
    i2c_device_add "$(get_mux_bus_num 8)" 0x58 ltc4282      # PEM1 Driver
    i2c_device_add "$(get_mux_bus_num 8)" 0x18 max6615      # PEM1 Driver
fi

# # i2c-mux 8, channel 2
is_pem2=$(
    pem-util pem2 --get_pem_info | grep WEDGE400-PEM
    echo $?
)
if [ "$is_pem2" = "1" ]; then
    i2c_device_add "$(get_mux_bus_num 9)" 0x58 psu_driver   # PSU2 Driver
else
    i2c_device_add "$(get_mux_bus_num 9)" 0x58 ltc4282      # PEM2 Driver
    i2c_device_add "$(get_mux_bus_num 9)" 0x18 max6615      # PEM2 Driver
fi

# # i2c-mux 8, channel 3
i2c_device_add "$(get_mux_bus_num 10)" 0x50 24c02          # BCM54616 EEPROM

# # i2c-mux 8, channel 4
i2c_device_add "$(get_mux_bus_num 11)" 0x50 24c02          # BCM54616 EEPROM

# # i2c-mux 8, channel 5
i2c_device_add "$(get_mux_bus_num 12)" 0x54 24c02          # TH3 EEPROM

# # i2c-mux 8, channel 6
i2c_device_add "$(get_mux_bus_num 13)" 0x50 pwrcpld_update # SMB PWRCPLD I2C Programming

# # i2c-mux 11, channel 1
i2c_device_add "$(get_mux_bus_num 16)" 0x3e fcbcpld # FCB CPLD

# # i2c-mux 11, channel 2
if [ "$brd_type" = "0" ] && [ "$brd_rev" = "0" ]; then  # WEDGE400 & EVT1
    i2c_device_add "$(get_mux_bus_num 17)" 0x51 24c02           # EEPROM
else
    i2c_device_add "$(get_mux_bus_num 17)" 0x51 24c64           # EEPROM
fi

# # i2c-mux 11, channel 3
i2c_device_add "$(get_mux_bus_num 18)" 0x48 tmp75           # Temp. sensor
i2c_device_add "$(get_mux_bus_num 18)" 0x49 tmp75           # Temp. sensor

# # i2c-mux 11, channel 4
i2c_device_add "$(get_mux_bus_num 19)" 0x10 adm1278         # FCB hot swap

# # i2c-mux 11, channel 5
i2c_device_add "$(get_mux_bus_num 20)" 0x52 24c64           # FAN tray

# # i2c-mux 11, channel 6
i2c_device_add "$(get_mux_bus_num 21)" 0x52 24c64           # FAN tray

# # i2c-mux 11, channel 7
i2c_device_add "$(get_mux_bus_num 22)" 0x52 24c64           # FAN tray

# # i2c-mux 11, channel 8
i2c_device_add "$(get_mux_bus_num 23)" 0x52 24c64           # FAN tray

#
# Check if I2C devices are bound to drivers. A summary message (total #
# of devices and # of devices without drivers) will be dumped at the end
# of this function.
#
i2c_check_driver_binding
