#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# Bus 0: Create i2c slave backend for ipmbd_0(BIC)
i2c_mslave_add 0 0x10

# Bus 4: Create i2c slave backend for ipmbd_4(Debug Card)
i2c_mslave_add 4 0x10

# Bus 16: i2c bus 1 mux, channel 0
i2c_device_add 16 0x4d ir35215              # ir35215
# Bus 17: i2c bus 1 mux, channel 1
i2c_device_add 17 0x40 xdpe132g5c           # xdpe132g5c
# Bus 18: i2c bus 1 mux, channel 2
i2c_device_add 18 0x3a powr1220             # powr1220
# Bus 19: i2c bus 1 mux, channel 3
i2c_device_add 19 0x68 xdpe12284            # xdpe12284
# Bus 20: i2c bus 1 mux, channel 4
i2c_device_add 20 0x0e pxe1211              # pxe1211
# Bus 21: i2c bus 1 mux, channel 5
i2c_device_add 21 0x47 ir35215              # ir35215
# Bus 22: i2c bus 1 mux, channel 6
i2c_device_add 22 0x60 xdpe12284            # xdpe12284
# Bus 23: i2c bus 1 mux, channel 7
# None

# Bus 2
i2c_device_add 2 0x3e scmcpld               # SCM CPLD

# Bus 24: i2c bus 2 mux, channel 0
i2c_device_add 24 0x10 adm1278              # adm1278
# Bus 26: i2c bus 2 mux, channel 2
# None
# Bus 27: i2c bus 2 mux, channel 3
i2c_device_add 27 0x52 24c64                # SCM EEPROM
# Bus 28: i2c bus 2 mux, channel 4
i2c_device_add 28 0x50 24c02                # SCM BCM546161S EEPROM
# Bus 29: i2c bus 2 mux, channel 5
# None
# Bus 30: i2c bus 2 mux, channel 6
# None
# Bus 31: i2c bus 2 mux, channel 7
# i2c_device_add 31 0x50 si53108            # si53108 do not enable this

# Bus 3
# Bus 35: i2c bus 3 mux, channel 3
i2c_device_add 35 0x4c tmp421               # tmp421
# Bus 36: i2c bus 3 mux, channel 4
i2c_device_add 36 0x4d tmp421               # tmp421
# Bus 38: i2c bus 3 mux, channel 6
i2c_device_add 38 0x2a net_asic             # GB switch i2c driver

# Bus 4
i2c_device_add 4 0x27 pca9555               # PCA9555 GPIO expander for debug card

# Bus 5
i2c_device_add 5 0x60 domfpga               # DOM FPGA 2

# Bus 6
i2c_device_add 6 0x51 24c64                 # SMB EEPROM
i2c_device_add 6 0x21 pca9534               # pca9534 GPIO expander

# Bus 7
# i2c_device_add 7 0x2e tpm_tis_i2c         # i2c tpm, driver binding in device tree

# Bus 8
i2c_device_add 8 0x51 24c64                 # i2c EEPROM
# Bus 40: i2c bus 8 mux, channel 0
# i2c_device_add 40 0xa0/0xb0 24c02/24c64   # PSU1 EEPROM
i2c_device_add 40 0x58 psu_driver           # PSU1 MCU
# Bus 41: i2c bus 8 mux, channel 1
# i2c_device_add 41 0xa0/0xb0 24c02/24c64   # PSU1 EEPROM
i2c_device_add 41 0x58 psu_driver           # PSU2 MCU
# Bus 42: i2c bus 8 mux, channel 2
i2c_device_add 42 0x50 24c02                # BMC BCM54616S EEPROM
# Bus 43: i2c bus 8 mux, channel 3
i2c_device_add 43 0x50 24c02                # BCM5389 BCM54616S EEPROM
# Bus 44: i2c bus 8 mux, channel 4
# None
# Bus 45: i2c bus 8 mux, channel 5
# pwr cpld upgrade interface
# Bus 46: i2c bus 8 mux, channel 6
# None
# Bus 47: i2c bus 8 mux, channel 7
i2c_device_add 47 0x3e smb_pwrcpld          # PWR CPLD

# Bus 9
# i2c_device_add 9 0x74 si5391b             # si5391b do not enable this
# i2c_device_add 9 TBD usb_hub              # usb_hub do not enable this

# Bus 10
# i2c_device_add 10 0x0d runbmc_fpga        # PFR FPGA

# Bus 12
i2c_device_add 12 0x3e smb_syscpld          # SMB CPLD

# Bus 13
i2c_device_add 13 0x60 domfpga              # DOM FPGA 1

# Bus 14
# disabled, multiple function pin, used as MDIO

# Bus 48: i2c bus 15 mux, channel 0
i2c_device_add 48 0x3e fcbcpld              # FCM CPLD
# Bus 49: i2c bus 15 mux, channel 1
i2c_device_add 49 0x51 24c64                # FCM EEPROM
# Bus 51: i2c bus 15 mux, channel 3
i2c_device_add 51 0x10 adm1278              # FCM Hotswap adm1278
# Bus 52: i2c bus 15 mux, channel 4
i2c_device_add 52 0x52 24c64                # FAN4 EEPROM
# Bus 53: i2c bus 15 mux, channel 5
i2c_device_add 53 0x52 24c64                # FAN3 EEPROM
# Bus 54: i2c bus 15 mux, channel 6
i2c_device_add 54 0x52 24c64                # FAN2 EEPROM
# Bus 55: i2c bus 15 mux, channel 7
i2c_device_add 55 0x52 24c64                # FAN1 EEPROM

# cloudripper will mix the NXP LM75 and TI TMP1075 sensor chips
# LM75 not support device id read, but TI TMP1075 support
# BMC can access TMP1075 0x0F address to get TMP1075 device id
TMP1075_ID=0x75
fixup_lm75_tmp1075_devices() {
    # Bus 25: i2c bus 2 mux, channel 1
    DEV_ID=$(i2cget -y -f 25 0x4c 0x0f)
    if [ "$DEV_ID" = "$TMP1075_ID" ]; then
        i2c_device_add 25 0x4c tmp1075              # i2c tmp1075
    else
        i2c_device_add 25 0x4c lm75                 # i2c lm75
    fi

    DEV_ID=$(i2cget -y -f 25 0x4d 0x0f)
    if [ "$DEV_ID" = "$TMP1075_ID" ]; then
        i2c_device_add 25 0x4d tmp1075              # i2c tmp1075
    else
        i2c_device_add 25 0x4d lm75                 # i2c lm75
    fi

    # Bus 3
    DEV_ID=$(i2cget -y -f 3 0x4a 0x0f)
    if [ "$DEV_ID" = "$TMP1075_ID" ]; then
        i2c_device_add 3 0x4a tmp1075               # i2c tmp1075
    else
        i2c_device_add 3 0x4a lm75                  # i2c lm75
    fi

    # Bus 32: i2c bus 3 mux, channel 0
    DEV_ID=$(i2cget -y -f 32 0x48 0x0f)
    if [ "$DEV_ID" = "$TMP1075_ID" ]; then
        i2c_device_add 32 0x48 tmp1075              # i2c tmp1075
    else
        i2c_device_add 32 0x48 lm75                 # i2c lm75
    fi
    # Bus 33: i2c bus 3 mux, channel 1
    DEV_ID=$(i2cget -y -f 33 0x49 0x0f)
    if [ "$DEV_ID" = "$TMP1075_ID" ]; then
        i2c_device_add 33 0x49 tmp1075              # i2c tmp1075
    else
        i2c_device_add 33 0x49 lm75                 # i2c lm75
    fi
    # Bus 34: i2c bus 3 mux, channel 2
    DEV_ID=$(i2cget -y -f 34 0x4b 0x0f)
    if [ "$DEV_ID" = "$TMP1075_ID" ]; then
        i2c_device_add 34 0x4b tmp1075              # tmp1075
    else
        i2c_device_add 34 0x4b lm75                 # i2c lm75
    fi

    # Bus 37: i2c bus 3 mux, channel 5
    DEV_ID=$(i2cget -y -f 37 0x4e 0x0f)
    if [ "$DEV_ID" = "$TMP1075_ID" ]; then
        i2c_device_add 37 0x4e tmp1075              # i2c tmp1075
    else
        i2c_device_add 37 0x4e lm75                 # i2c lm75
    fi

    # Bus 39: i2c bus 3 mux, channel 7
    DEV_ID=$(i2cget -y -f 39 0x4f 0x0f)
    if [ "$DEV_ID" = "$TMP1075_ID" ]; then
        i2c_device_add 39 0x4f tmp1075              # i2c tmp1075
    else
        i2c_device_add 39 0x4f lm75                 # i2c lm75
    fi

    # Bus 8
    DEV_ID=$(i2cget -y -f 8 0x4a 0x0f)
    if [ "$DEV_ID" = "$TMP1075_ID" ]; then
        i2c_device_add 8 0x4a tmp1075               # i2c tmp1075
    else
        i2c_device_add 8 0x4a lm75                  # i2c lm75
    fi

    # Bus 50: i2c bus 15 mux, channel 2
    DEV_ID=$(i2cget -y -f 50 0x48 0x0f)
    if [ "$DEV_ID" = "$TMP1075_ID" ]; then
        i2c_device_add 50 0x48 tmp1075              # FCM tmp1075
    else
        i2c_device_add 50 0x48 lm75                 # FCM lm75
    fi

    DEV_ID=$(i2cget -y -f 50 0x49 0x0f)
    if [ "$DEV_ID" = "$TMP1075_ID" ]; then
        i2c_device_add 50 0x49 tmp1075              # FCM tmp1075
    else
        i2c_device_add 50 0x49 lm75                 # FCM lm75
    fi
}

fixup_lm75_tmp1075_devices