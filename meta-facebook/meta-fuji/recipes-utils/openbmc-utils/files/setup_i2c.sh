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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
#shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

# Board Version EVT2 0x41, EVT3 0x42, DVT1 0x43
BOARD_VER=$(i2cget -f -y 13 0x35 0x3 | awk '{printf "%d", $1}') #Get board version

# # Bus 0
i2c_device_add 0 0x1010 slave-mqueue #IPMB 0

# # Bus 1
i2c_device_add 1 0x40 xdpe132g5c #ISL68137 DC-DC core
i2c_device_add 1 0x53 mp2978
i2c_device_add 1 0x59 mp2978

# # Bus 2
i2c_device_add 2 0x35 scmcpld  #SCM CPLD

# # Bus 4
i2c_device_add 4 0x1010 slave-mqueue #IPMB 1
i2c_device_add 4 0x27 smb_debugcardcpld  # SMB DEBUGCARD CPLD

# # Bus 13
i2c_device_add 13 0x35 iobfpga #IOB FPGA

# # i2c-mux 2, channel 2
i2c_device_add 17 0x4c lm75   #SCM temp. sensor
i2c_device_add 17 0x4d lm75   #SCM temp. sensor

# # i2c-mux 2, channel 3
i2c_device_add 19 0x52 24c64   #EEPROM

# # i2c-mux 2, channel 4
i2c_device_add 20 0x50 24c02   #BMC54616S EEPROM

# # i2c-mux 2, channel 6
i2c_device_add 22 0x52 24c02   #BMC54616S EEPROM

# # Bus 3
i2c_device_add 3 0x48 lm75     #LM75B_1# Thermal sensor
i2c_device_add 3 0x49 lm75     #LM75B_2# Thermal sensor
i2c_device_add 3 0x4a lm75     #LM75B_3# Thermal sensor
i2c_device_add 3 0x4c tmp422   #TMP422# Thermal sensor(TH4)

# # Bus 5
i2c_device_add 5 0x35 ucd90160		  # Power Sequence
i2c_device_add 5 0x36 ucd90160		  # Power Sequence

# Bus 8
i2c_device_add 8 0x51 24c64
i2c_device_add 8 0x4a lm75

# net_brcm driver only support DVT1 and later
if [ "$BOARD_VER" -gt 66 ];then
    i2c_device_add 29 0x47 net_brcm
fi

# # i2c-mux PCA9548 0x70, channel 1, mux PCA9548 0x71
i2c_device_add 48 0x58 psu_driver
i2c_device_add 49 0x5a psu_driver
i2c_device_add 50 0x4c lm75     #SIM
i2c_device_add 50 0x52 24c64 	#SIM
i2c_device_add 51 0x48 tmp75
i2c_device_add 52 0x49 tmp75
i2c_device_add 54 0x21 pca9534	#PCA9534
if i2cget -y -f 55 0x60 > /dev/null;then
    i2c_device_add 55 0x60 smb_pwrcpld 	#PDB-L
else
    i2c_device_add 53 0x60 smb_pwrcpld 	#PDB-L
fi

# # i2c-mux PCA9548 0x70, channel 2, mux PCA9548 0x72
i2c_device_add 56 0x58 psu_driver 	#PSU4
i2c_device_add 57 0x5a psu_driver 	#PSU3

i2c_device_add 59 0x48 tmp75
i2c_device_add 60 0x49 tmp75
i2c_device_add 62 0x21 pca9534
if i2cget -y -f 63 0x60 > /dev/null;then
    i2c_device_add 63 0x60 smb_pwrcpld 	#PDB-R
else
    i2c_device_add 61 0x60 smb_pwrcpld  #PDB-R
fi

# # i2c-mux PCA9548 0x70, channel 3, mux PCA9548 0x76
i2c_device_add 64 0x33	fcbcpld #CPLD
i2c_device_add 65 0x53	24c64
i2c_device_add 66 0x49 tmp75
i2c_device_add 66 0x48 tmp75
i2c_device_add 68 0x52 24c64    #fan 7 eeprom
i2c_device_add 69 0x52 24c64    #fan 5 eeprom
i2c_device_add 70 0x52 24c64    #fan 3 eeprom
i2c_device_add 71 0x52 24c64    #fan 1 eeprom

# # i2c-mux PCA9548 0x70, channel 4, mux PCA9548 0x76
i2c_device_add 72 0x33	fcbcpld #FCM CPLD
i2c_device_add 73 0x53	24c64
i2c_device_add 74 0x49 tmp75
i2c_device_add 74 0x48 tmp75
i2c_device_add 76 0x52 24c64    #fan 8 eeprom
i2c_device_add 77 0x52 24c64    #fan 6 eeprom
i2c_device_add 78 0x52 24c64    #fan 4 eeprom
i2c_device_add 79 0x52 24c64    #fan 2 eeprom
# # i2c-mux PCA9548 0x70, channel 5
i2c_device_add 28 0x50 24c02    #BMC54616S EEPROM

#
# 11-0077 (channel #1) -> 40-0076: 8 channels (i2c-80 -> i2c87) connecting
# to PIM #1.
#
i2c_device_add 80 0x60 domfpga  # DOM FPGA
i2c_device_add 81 0x56 24c64
i2c_device_add 82 0x48 tmp75
i2c_device_add 83 0x4b tmp75
i2c_device_add 84 0x4a tmp75
i2c_device_add 84 0x10 adm1278
i2c_device_add 86 0x34 ucd90160
i2c_device_add 86 0x6b mp2975

#
# 11-0077 (channel #2) -> 41-0076: 8 channels (i2c-88 -> i2c-95) connecting
# to PIM #2.
#
i2c_device_add 88 0x60 domfpga 	# DOM FPGA
i2c_device_add 89 0x56 24c64
i2c_device_add 90 0x48 tmp75
i2c_device_add 91 0x4b tmp75
i2c_device_add 92 0x4a tmp75
i2c_device_add 92 0x10 adm1278
i2c_device_add 94 0x34 ucd90160
i2c_device_add 94 0x6b mp2975

#
# 11-0077 (channel #3) -> 42-0076: 8 channels (i2c-96 -> i2c-103) connecting
# to PIM #3.
#
i2c_device_add 96 0x60 domfpga 	    # DOM FPGA
i2c_device_add 97 0x56 24c64
i2c_device_add 98 0x48 tmp75
i2c_device_add 99 0x4b tmp75
i2c_device_add 100 0x4a tmp75
i2c_device_add 100 0x10 adm1278
i2c_device_add 102 0x34 ucd90160
i2c_device_add 102 0x6b mp2975

#
# 11-0077 (channel #4) -> 43-0076: 8 channels (i2c-104 -> i2c-111) connecting
# to PIM #4.
#
i2c_device_add 104 0x60 domfpga 	# DOM FPGA
i2c_device_add 105 0x56 24c64
i2c_device_add 106 0x48 tmp75
i2c_device_add 107 0x4b tmp75
i2c_device_add 108 0x4a tmp75
i2c_device_add 108 0x10 adm1278
i2c_device_add 110 0x34 ucd90160
i2c_device_add 110 0x6b mp2975

#
# 11-0077 (channel #5) -> 44-0076: 8 channels (i2c-112 -> i2c-119) connecting
# to PIM #5.
#
i2c_device_add 112 0x60 domfpga 	# DOM FPGA
i2c_device_add 113 0x56 24c64
i2c_device_add 114 0x48 tmp75
i2c_device_add 115 0x4b tmp75
i2c_device_add 116 0x4a tmp75
i2c_device_add 116 0x10 adm1278
i2c_device_add 118 0x34 ucd90160
i2c_device_add 118 0x6b mp2975

#
# 11-0077 (channel #6) -> 45-0076: 8 channels (i2c-120 -> i2c-127) connecting
# to PIM #6.
#
i2c_device_add 120 0x60 domfpga 	# DOM FPGA
i2c_device_add 121 0x56 24c64
i2c_device_add 122 0x48 tmp75
i2c_device_add 123 0x4b tmp75
i2c_device_add 124 0x4a tmp75
i2c_device_add 124 0x10 adm1278
i2c_device_add 126 0x34 ucd90160
i2c_device_add 126 0x6b mp2975

#
# 11-0077 (channel #7) -> 46-0076: 8 channels (i2c-128 -> i2c-135) connecting
# to PIM #7.
#
i2c_device_add 128 0x60 domfpga 	# DOM FPGA
i2c_device_add 129 0x56 24c64
i2c_device_add 130 0x48 tmp75
i2c_device_add 131 0x4b tmp75
i2c_device_add 132 0x4a tmp75
i2c_device_add 132 0x10 adm1278
i2c_device_add 134 0x34 ucd90160
i2c_device_add 134 0x6b mp2975

#
# 11-0077 (channel #8) -> 47-0076: 8 channels (i2c-136 -> i2c-143) connecting
# to PIM #8.
#
i2c_device_add 136 0x60 domfpga 	# DOM FPGA
i2c_device_add 137 0x56 24c64
i2c_device_add 138 0x48 tmp75
i2c_device_add 139 0x4b tmp75
i2c_device_add 140 0x4a tmp75
i2c_device_add 140 0x10 adm1278
i2c_device_add 142 0x34 ucd90160
i2c_device_add 142 0x6b mp2975


# # Bus 12
i2c_device_add 12 0x3e smb_syscpld     # SYSTEM CPLD

#
# Check if I2C devices are bound to drivers. A summary message (total #
# of devices and # of devices without drivers) will be dumped at the end
# of this function.
#
i2c_check_driver_binding "fix-binding"
