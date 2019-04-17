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

board_rev=$(wedge_board_rev)

KERNEL_VERSION=`uname -r`
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
#    i2c-mux 2-0070:  child bus 14-21
#    i2c-mux 8-0070:  child bus 22-29
#    i2c-mux 11-0070: child bus 30-37
    start_of_mux_bus=14
else
    start_of_mux_bus=16
fi

get_mux_bus_num()
{
    echo $((${start_of_mux_bus}+$1))
}

# # Bus 0
i2c_device_add 0 0x20 com_e_driver     # COM-e

# # Bus 2
i2c_device_add 2 0x3e scmcpld          # SCMCPLD

# # Bus 12
i2c_device_add 12 0x3e smb_syscpld     # SMB_SYSCPLD

# # i2c-mux 8, channel 7
i2c_device_add $(get_mux_bus_num 15) 0x3e smb_pwrcpld # SMB_PWRCPLD

# # Bus 13
i2c_device_add 13 0x32 domfpga         # DOM FPGA 1

# # Bus 4
i2c_device_add 4 0x27 pca9555          # PCA9555

# # Bus 5
i2c_device_add 5 0x34 domfpga          # DOM FPGA 2

# # Bus 6
i2c_device_add 6 0x51 24c64            # SMB EEPROM
i2c_device_add 6 0x27 pca9534          # PCA9534
i2c_device_add 6 0x20 pca9535          # PCA9535

# # Bus 9
i2c_device_add 9 0x68 si5391b          # SI5391B
i2c_device_add 9 0x2c usb2513          # USB2513

# # Bus 1
i2c_device_add 1 0x3a powr1220         # SMB power sequencer
i2c_device_add 1 0x13 ir35215          # TH3 core voltage/current monitor
i2c_device_add 1 0x14 ir35215          # TH3 core voltage/current monitor
i2c_device_add 1 0x60 isl68137         # TH3 serdes voltage/current monitor


# # Bus 3
i2c_device_add 3 0x48 tmp75            # SMB temp. sensor
i2c_device_add 3 0x49 tmp75            # SMB temp. sensor
i2c_device_add 3 0x4a tmp75            # SMB temp. sensor
i2c_device_add 3 0x4c tmp421           # SMB temp. sensor
i2c_device_add 3 0x4d tmp421           # SMB temp. sensor
i2c_device_add 3 0x4f tmp421           # SMB temp. sensor

# # i2c-mux 2, channel 1
i2c_device_add $(get_mux_bus_num 0) 0x10 adm1278  # SCM Hotswap

# # i2c-mux 2, channel 2
i2c_device_add $(get_mux_bus_num 1) 0x4c tmp75           # SCM temp. sensor
i2c_device_add $(get_mux_bus_num 1) 0x4d tmp75           # SCM temp. sensor

# # i2c-mux 2, channel 4
i2c_device_add $(get_mux_bus_num 3) 0x52 24c64           # EEPROM

# # i2c-mux 2, channel 5
i2c_device_add $(get_mux_bus_num 4) 0x4d 24c02           # BMC54616S EEPROM

# # i2c-mux 2, channel 6
i2c_device_add $(get_mux_bus_num 5) 0x50 nvme            # NVME

# # i2c-mux 2, channel 8
i2c_device_add $(get_mux_bus_num 7) 0x5b si53108         # PCIE clock buffer

# # i2c-mux 8, channel 1
i2c_device_add $(get_mux_bus_num 8) 0x58 psu_driver      # PSU1 Driver

# # i2c-mux 8, channel 2
i2c_device_add $(get_mux_bus_num 9) 0x58 psu_driver      # PSU2 Driver

# # i2c-mux 8, channel 3
i2c_device_add $(get_mux_bus_num 10) 0x50 24c02           # BCM54616 EEPROM

# # i2c-mux 8, channel 4
i2c_device_add $(get_mux_bus_num 11) 0x50 24c02           # BCM54616 EEPROM

# # i2c-mux 8, channel 5
i2c_device_add $(get_mux_bus_num 12) 0x54 24c02           # TH3 EEPROM

# # i2c-mux 8, channel 6
i2c_device_add $(get_mux_bus_num 13) 0x38 pwrcpld_update  # SMB PWRCPLD I2C Programming

# # i2c-mux 11, channel 1
i2c_device_add $(get_mux_bus_num 16) 0x3e fcbcpld         # FCB CPLD

# # i2c-mux 11, channel 2
i2c_device_add $(get_mux_bus_num 17) 0x51 24c02           # EEPROM

# # i2c-mux 11, channel 3
i2c_device_add $(get_mux_bus_num 18) 0x48 tmp75           # Temp. sensor
i2c_device_add $(get_mux_bus_num 18) 0x49 tmp75           # Temp. sensor

# # i2c-mux 11, channel 4
i2c_device_add $(get_mux_bus_num 19) 0x10 adm1278         # FCB hot swap

# # i2c-mux 11, channel 5
i2c_device_add $(get_mux_bus_num 20) 0x52 24c64           # FAN tray

# # i2c-mux 11, channel 6
i2c_device_add $(get_mux_bus_num 21) 0x52 24c64           # FAN tray

# # i2c-mux 11, channel 7
i2c_device_add $(get_mux_bus_num 22) 0x52 24c64           # FAN tray

# # i2c-mux 11, channel 8
i2c_device_add $(get_mux_bus_num 23) 0x52 24c64           # FAN tray
