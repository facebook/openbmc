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

# # Bus 2
i2c_device_add 2 0x35 scmcpld          # SCMCPLD

# # Bus 12
i2c_device_add 12 0x3e smbcpld         # SMBCPLD

# # Bus 1
i2c_device_add 1 0x3a powr1220         # SMB power sequencer
i2c_device_add 1 0x12 ir3595           # TH3 core voltage/current monitor
i2c_device_add 1 0x60 isl68127         # TH3 serdes voltage/current monitor

# # Bus 3
i2c_device_add 3 0x48 tmp75            # SMB temp. sensor
i2c_device_add 3 0x49 tmp75            # SMB temp. sensor
i2c_device_add 3 0x4a tmp75            # SMB temp. sensor
i2c_device_add 3 0x4b tmp75            # SMB temp. sensor

# # Bus 30
i2c_device_add 30 0x4c tmp422          # TH3 temp. sensor

# # Bus 16
i2c_device_add 16 0x10 adm1278         # SCM Hotswap

# # Bus 17
i2c_device_add 17 0x4c tmp421          # SCM temp. sensor
i2c_device_add 17 0x4d tmp421          # SCM temp. sensor

# # Bus 49
i2c_device_add 49 0x59 pfe1100         # PDB-L PFE1100 PSU

# # Bus 48
i2c_device_add 48 0x58 pfe1100         # PDB-L PFE1100 PSU

# # Bus 57
i2c_device_add 57 0x59 pfe1100         # PDB-R PFE1100 PSU

# # Bus 56
i2c_device_add 56 0x58 pfe1100         # PDB-R PFE1100 PSU

# # Bus 51
i2c_device_add 51 0x48 tmp75           # PDB-L temp. sensor

# # Bus 52
i2c_device_add 52 0x49 tmp75           # PDB-L temp. sensor

# # Bus 59
i2c_device_add 59 0x48 tmp75           # PDB-R temp. sensor

# # Bus 60
i2c_device_add 60 0x49 tmp75           # PDB-R temp. sensor

# # Bus 64
i2c_device_add 64 0x33 fcmcpld         # Top FCMCPLD

# # Bus 72
i2c_device_add 72 0x33 fcmcpld         # Bottom FCMCPLD

# # Bus 66
i2c_device_add 66 0x48 tmp75           # FCM-T temp. sensor
i2c_device_add 66 0x49 tmp75           # FCM-T temp. sensor

# # Bus 67
i2c_device_add 67 0x10 adm1278         # FCM-T Hotswap

# # Bus 74
i2c_device_add 74 0x48 tmp75           # FCM-B temp. sensor
i2c_device_add 74 0x49 tmp75           # FCM-B temp. sensor

# # Bus 67
i2c_device_add 75 0x10 adm1278         # FCM-T Hotswap

# # Bus 84 92 100 108 116 124 132 140  # PIM1 ~ PIM8 Hotswap
BUS="84 92 100 108 116 124 132 140"
for bus in ${BUS}; do
    i2c_device_add $bus 0x10 adm1278
done

# # Bus 86 94 102 110 118 126 134 142  # PIM1 ~ PIM8 MAX34461
BUS="86 94 102 110 118 126 134 142"
for bus in ${BUS}; do
    i2c_device_add $bus 0x74 max34461
done

# # Bus 82 90 98 106 114 122 130 138   # PIM1 ~ PIM8 temp. sensor
# # Bus 83 91 99 107 115 123 131 139   # PIM1 ~ PIM8 temp. sensor
BUS="82 90 98 106 114 122 130 138"
for bus in ${BUS}; do
    i2c_device_add $bus 0x48 tmp75
    i2c_device_add $(($bus+1)) 0x4b tmp75
done

# # Bus 6
i2c_device_add 6 0x51 24c64            # Board fru EEPROM

# # Bus 18
i2c_device_add 18 0x52 24c64           # SCM fru eeprom

# # Bus 65
i2c_device_add 65 0x51 24c02           # FCM-T fru eeprom

# # Bus 73
i2c_device_add 73 0x51 24c02           # FCM-B fru eeprom

# # Bus 68 ~ 71                        # Fantray 7 5 3 1  fru eeprom
# # Bus 76 ~ 79                        # Fantray 8 6 4 2  fru eeprom
BUS="68 69 70 71 76 77 78 79"
for bus in ${BUS}; do
    i2c_device_add $bus 0x52 24c64
done

# # Bus 81 89 97 105 113 121 129 137   # PIM1 ~ PIM8 fru eeprom
BUS="81 89 97 105 113 121 129 137"
for bus in ${BUS}; do
    i2c_device_add $bus 0x53 24c64     # 4DD
    i2c_device_add $bus 0x56 24c64     # 16Q
done
