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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

get_mux_bus_num() {
    i2c_switch=$1
    channel=$2

    if [ "$i2c_switch" = "2-0070" ]; then
        start_bus_num=16
    elif [ "$i2c_switch" = "8-0070" ]; then
        start_bus_num=24
    elif [ "$i2c_switch" = "11-0076" ]; then
        start_bus_num=32
    else
        echo "Error: i2c switch $i2c_switch does not exist"
        start_bus_num=200
    fi

    echo $((start_bus_num + channel))
}

brd_type=$(wedge_board_type)
brd_rev=$(wedge_board_rev)
brd_type_rev=$(wedge_board_type_rev)

# # Bus 2
i2c_device_add 2 0x3e scmcpld          # SCMCPLD

# # Bus 12
i2c_device_add 12 0x3e smb_syscpld     # SMB_SYSCPLD

# # i2c-mux 8-0070, channel 8
i2c_device_add "$(get_mux_bus_num 8-0070 7)" 0x3e smb_pwrcpld # SMB_PWRCPLD

# # Bus 13
i2c_device_add 13 0x60 domfpga        # DOM FPGA 1

# # Bus 4
i2c_device_add 4 0x27 pca9555         # PCA9555

# # Bus 5
i2c_device_add 5 0x60 domfpga         # DOM FPGA 2

# # Bus 6
i2c_device_add 6 0x51 24c64            # SMB EEPROM
i2c_device_add 6 0x21 pca9534          # PCA9534

if [ "$brd_type" = "0" ] && [ $((brd_rev)) -ge 6 ]; then
    i2c_device_add 6 0x50 24c64            # RACKMON EEPROM
fi

# # Bus 1
i2c_device_add 1 0x3a powr1220          # SMB power sequencer
i2c_device_add 1 0x4d ir35215           # TH3 serdes voltage/current monitor on the left
i2c_device_add 1 0x47 ir35215           # TH3 serdes voltage/current monitor on the right

# # Bus 3
i2c_device_add 3 0x48 tmp75            # SMB temp. sensor
i2c_device_add 3 0x4b tmp75            # SMB temp. sensor

if [ "$brd_type_rev" != "WEDGE400_MP_RESPIN" ]; then
    i2c_device_add 3 0x49 tmp75            # SMB temp. sensor
    i2c_device_add 3 0x4a tmp75            # SMB temp. sensor
    i2c_device_add 3 0x4c tmp421           # SMB temp. sensor
    i2c_device_add 3 0x4e tmp421           # SMB temp. sensor
fi

# # i2c-mux 2-0070, channel 1, support dual footprint
if i2cget -y -f "$(get_mux_bus_num 2-0070 0)" 0x10 &> /dev/null; then
    i2c_device_add "$(get_mux_bus_num 2-0070 0)" 0x10 adm1278 # SCM Hotswap
else
    i2c_device_add "$(get_mux_bus_num 2-0070 0)" 0x44 lm25066 # SCM Hotswap
fi

# # i2c-mux 2-0070, channel 2
if [ "$brd_type_rev" != "WEDGE400_MP_RESPIN" ]; then
    i2c_device_add "$(get_mux_bus_num 2-0070 1)" 0x4c tmp75   # SCM temp. sensor
fi

i2c_device_add "$(get_mux_bus_num 2-0070 1)" 0x4d tmp75   # SCM temp. sensor

# # i2c-mux 2-0070, channel 4
i2c_device_add "$(get_mux_bus_num 2-0070 3)" 0x52 24c64   # EEPROM

# # i2c-mux 2, channel 5
i2c_device_add "$(get_mux_bus_num 2-0070 4)" 0x50 24c02   # BMC54616S EEPROM

# # i2c-mux 8-0070, channel 1
if [ "$(wedge_power_supply_type 1)" = "PSU" ] ||
   [ "$(wedge_power_supply_type 1)" = "PSU48" ]; then
    i2c_device_add "$(get_mux_bus_num 8-0070 0)" 0x58 psu_driver   # PSU1 Driver
else
    i2c_device_add "$(get_mux_bus_num 8-0070 0)" 0x58 ltc4282      # PEM1 Driver
    i2c_device_add "$(get_mux_bus_num 8-0070 0)" 0x18 max6615      # PEM1 Driver
fi

# # i2c-mux 8-0070, channel 2
if [ "$(wedge_power_supply_type 2)" = "PSU" ] ||
   [ "$(wedge_power_supply_type 2)" = "PSU48" ]; then
    i2c_device_add "$(get_mux_bus_num 8-0070 1)" 0x58 psu_driver   # PSU2 Driver
else
    i2c_device_add "$(get_mux_bus_num 8-0070 1)" 0x58 ltc4282      # PEM2 Driver
    i2c_device_add "$(get_mux_bus_num 8-0070 1)" 0x18 max6615      # PEM2 Driver
fi

#
# For wedge400 MP respin and later.
# Dual footprint will mount either one temp sensor for each bus.
# i2c-mux 8-0070, channel 3-5 and 7
#

if [ "$brd_type" = "0" ] && [ $((brd_rev)) -ge 6 ]; then
    # # i2c-mux 8-0070, channel 3
    if i2cget -y -f "$(get_mux_bus_num 8-0070 2)" 0x4d &> /dev/null; then
        i2c_device_add "$(get_mux_bus_num 8-0070 2)" 0x4d tmp421      # TMP421_4
    else
        i2c_device_add "$(get_mux_bus_num 8-0070 2)" 0x4c adm1032     # ADM1032_4
    fi
    # # i2c-mux 8-0070, channel 4
    if i2cget -y -f "$(get_mux_bus_num 8-0070 3)" 0x4d &> /dev/null; then
        i2c_device_add "$(get_mux_bus_num 8-0070 3)" 0x4d tmp421      # TMP421_3
    else
        i2c_device_add "$(get_mux_bus_num 8-0070 3)" 0x4c adm1032     # ADM1032_3
    fi
    # # i2c-mux 8-0070, channel 5
    if i2cget -y -f "$(get_mux_bus_num 8-0070 4)" 0x4d &> /dev/null; then
        i2c_device_add "$(get_mux_bus_num 8-0070 4)" 0x4d tmp421      # TMP421_2
    else
        i2c_device_add "$(get_mux_bus_num 8-0070 4)" 0x4c adm1032     # ADM1032_2
    fi
    # # i2c-mux 8-0070, channel 7
    if i2cget -y -f "$(get_mux_bus_num 8-0070 6)" 0x4d &> /dev/null; then
        i2c_device_add "$(get_mux_bus_num 8-0070 6)" 0x4d tmp421      # TMP421_1
    else
        i2c_device_add "$(get_mux_bus_num 8-0070 6)" 0x4c adm1032     # ADM1032_1
    fi
fi
#
# End dual footprint temperature sensors
#

# # i2c-mux 8-0070, channel 5
if [ "$brd_type_rev" != "WEDGE400_MP_RESPIN" ]; then
    i2c_device_add "$(get_mux_bus_num 8-0070 4)" 0x54 24c02       # TH3 EEPROM
fi

# # i2c-mux 11-0076, channel 1
i2c_device_add "$(get_mux_bus_num 11-0076 0)" 0x3e fcbcpld # FCB CPLD

# # i2c-mux 11-0076, channel 2
i2c_device_add "$(get_mux_bus_num 11-0076 1)" 0x51 24c64           # FCM EEPROM

# # i2c-mux 11-0076, channel 3
i2c_device_add "$(get_mux_bus_num 11-0076 2)" 0x48 tmp75           # Temp. sensor
i2c_device_add "$(get_mux_bus_num 11-0076 2)" 0x49 tmp75           # Temp. sensor

# # i2c-mux 11-0076, channel 4, support dual footprint
if i2cget -y -f "$(get_mux_bus_num 11-0076 3)" 0x10 &> /dev/null; then
    i2c_device_add "$(get_mux_bus_num 11-0076 3)" 0x10 adm1278     # FCB hot swap
else
    i2c_device_add "$(get_mux_bus_num 11-0076 3)" 0x44 lm25066     # FCB hot swap
fi

# # i2c-mux 11-0076, channel 5
i2c_device_add "$(get_mux_bus_num 11-0076 4)" 0x52 24c64           # FAN tray

# # i2c-mux 11-0076, channel 6
i2c_device_add "$(get_mux_bus_num 11-0076 5)" 0x52 24c64           # FAN tray

# # i2c-mux 11-0076, channel 7
i2c_device_add "$(get_mux_bus_num 11-0076 6)" 0x52 24c64           # FAN tray

# # i2c-mux 11-0076, channel 8
i2c_device_add "$(get_mux_bus_num 11-0076 7)" 0x52 24c64           # FAN tray

fixup_wedge400_wedge400c_driver() {
    if [ "$brd_type" = "0" ]; then          # Only Wedge400
        i2c_device_add 1 0x60 isl68137      # TH3 core voltage/current monitor
        if [ "$brd_type_rev" != "WEDGE400_MP_RESPIN" ]; then
            i2c_device_add 3 0x4f tmp422        # TH3 temp. sensor
        fi
    elif [ "$brd_type" = "1" ]; then        # Only Wedge400-2
        i2c_device_add 1 0x40 xdpe132g5c    # Wedge400-2 GB core voltage/current monitor
        i2c_device_add 3 0x2a net_asic        # GB temp. sensor
        if [ "$brd_rev" = "0" ]; then
            i2c_device_add 1 0x43 ir35215   # Wedge400-2 GB serdes voltage/current monitor
        else
            i2c_device_add 1 0x0e pxe1211   # Wedge400-2 EVT2 or later GB serdes voltage/current monitor
        fi
    fi

    # BCM54616 EEPROMs are removed physically on Wedge400-C DVT units
    if [ "$brd_type_rev" != "WEDGE400-C_DVT" ]; then
        if [ "$brd_type_rev" != "WEDGE400_MP_RESPIN" ]; then
            # # i2c-mux 8-0070, channel 3
            i2c_device_add "$(get_mux_bus_num 8-0070 2)" 0x50 24c02          # BCM54616 EEPROM
            # # i2c-mux 8-0070, channel 4
            i2c_device_add "$(get_mux_bus_num 8-0070 3)" 0x50 24c02          # BCM54616 EEPROM
        fi
    fi

    # Wedge400 DVT2 or later, wedge400c EVT2 or later has BSM eeprom
    if [[ "$brd_type" -eq 0 && "$brd_rev" -ge 3 ]] || [[ "$brd_type" -eq 1 && "$brd_rev" -ge 1 ]]; then
        i2c_device_add "$(get_mux_bus_num 2-0070 6)" 0x56 24c64   # BSM EEPROM
    fi
}

fixup_wedge400_wedge400c_driver

#
# Check if I2C devices are bound to drivers. A summary message (total #
# of devices and # of devices without drivers) will be dumped at the end
# of this function.
#
i2c_check_driver_binding

# Hack only needed for wedge400c.
if [ "$brd_type" = "1" ]; then
    /usr/local/bin/rebind-driver.sh
fi
