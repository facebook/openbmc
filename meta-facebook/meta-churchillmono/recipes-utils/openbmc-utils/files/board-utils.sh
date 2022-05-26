#!/bin/bash
#
# Copyright (c) 2022 Cisco Systems Inc.
# Copyright (c) Meta Platforms, Inc. and affiliates.
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
# shellcheck disable=SC1091

# The Marvell GW swith reset and the Marvell GW switch EEPROM I2C access are controlled by writing to the CPU CPLD registers.
# To write to a CPU CLPD rigister:
# 1- First the CPLD register address is specified by writing to I2C addresses 0x31 to 0x34.
# 2- Then the data is specified by writing to addresses 0x35 to 0x38.
# 3- And finally the write request is triggered by writing to address 0x30 as below:
#
#0x30 is command  1: read,  0: write
#0x31 addr[7:0]
#0x32 addr[15:8]
#0x33 addr[23:16]
#0x34 addr[31:24]
#0x35 data[7:0]
#0x36 data[15:8]
#0x37 data[23:16]
#0x38 data[31:24]"

# To enable I2C access to Marvell GW switch EEPROM we are writing to register 0xC ("Misc. Control Register" in CPLD register map spec.).
# Setting bit 3 ("geSwI2cSel" in CPLD register map spec.) enables EEPROM access. The other bits are not modified from default values.

# enable access to GE switch EEPROM
oob_switch_eeprom_select() {

    i2cset -f -y 12 0x31 0x0c
    i2cset -f -y 12 0x32 0x00
    i2cset -f -y 12 0x33 0x00
    i2cset -f -y 12 0x34 0x00
    i2cset -f -y 12 0x35 0x07
    i2cset -f -y 12 0x36 0x2a
    i2cset -f -y 12 0x30 0x0
}

# To enable I2C access to Marveell GW switch EEPROM we are writing to register 0xC ("Misc. Control Register" in CPLD register map spec.).
# Clearing bit 2 ("geSwI2cSel" in CPLD register map spec.) disable EEPROM access. The other bits are not modified from default values.

# disable access to GE switch EEPROM
oob_switch_eeprom_deselect() {

    i2cset -f -y 12 0x31 0x0c
    i2cset -f -y 12 0x32 0x00
    i2cset -f -y 12 0x33 0x00
    i2cset -f -y 12 0x34 0x00
    i2cset -f -y 12 0x35 0x03
    i2cset -f -y 12 0x36 0x2a
    i2cset -f -y 12 0x30 0x0
}

# For switch reset we are writing to register 0xC ("Misc. Control Register" in CPLD register map spec.).
# 1- First, setting bit 8 ("gwSwRst" in CPLD register map spec.) to put the switch in reset. The other bits are not modified from default values.
# 2- After 0.5 second, bit 8 ("gwSwRst" in CPLD register map spec.) is cleared to bring the switch out of reset.

# GE switch reset
oob_switch_reset() {

    i2cset -f -y 12 0x31 0x0c
    i2cset -f -y 12 0x32 0x00
    i2cset -f -y 12 0x33 0x00
    i2cset -f -y 12 0x34 0x00
    i2cset -f -y 12 0x35 0x03
    i2cset -f -y 12 0x36 0x2b
    i2cset -f -y 12 0x30 0x0
    sleep 0.5
    i2cset -f -y 12 0x31 0x0c
    i2cset -f -y 12 0x32 0x00
    i2cset -f -y 12 0x33 0x00
    i2cset -f -y 12 0x34 0x00
    i2cset -f -y 12 0x35 0x03
    i2cset -f -y 12 0x36 0x2a
    i2cset -f -y 12 0x30 0x0
}

# read mac address from eeprom
read_mac_from_eeprom() {

    mac=00:00:00:00:00:00

    if [  -f  "/opt/cisco/bin/idprom" ]; then

        temp_mac=$(idprom /sys/bus/i2c/devices/8-0054/eeprom | grep MACADDR_BASE | awk '{print $2}')
        if [ -z "$temp_mac" ]; then
            echo "mac address not found"
        else
            mac=$(sed -e "s/\(..\)\(..\)\(..\)\(..\)\(..\)\(..\)/\1:\2:\3:\4:\5:\6/" <<< "$temp_mac")
        fi

    else

        # Temporary workaround until we get the idprom utility via PR for BSP
        hexdump -ve '1/1 "%.2x"' /sys/bus/i2c/devices/8-0054/eeprom > /tmp/eeprom_data

        ((i = 0))
        while [[ $i -lt 512 ]]
        do
            var=$( dd skip=$i count=4 if=/tmp/eeprom_data bs=1 2> /dev/null )
            if [ "$var" = "cf06" ];then
                ((i = i + 4))
                mac1=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)
                ((i = i + 2))
                mac2=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)
                ((i = i + 2))
                mac3=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)
                ((i = i + 2))
                mac4=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)
                ((i = i + 2))
                mac5=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)
                ((i = i + 2))
                mac6=$( dd skip=$i count=2 if=/tmp/eeprom_data bs=1 2> /dev/null)

                mac=$mac1:$mac2:$mac3:$mac4:$mac5:$mac6
                break
            fi
            ((i = i + 1))
        done
        rm /tmp/eeprom_data
    fi

    echo "$mac"
}
