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

#
# instantiate all the i2c-muxes.
# Note:
#   - the step is not needed in kernel 4.1 because all the i2c-muxes
#     are created in 4.1 kernel code.
#   - please do not modify the order of i2c-mux creation unless you've
#     decided to fix bus-number of leaf i2c-devices.
#
bulk_create_i2c_mux() {
    i2c_mux_name="pca9548"

    # The first-level i2c-muxes which are directly connected to aspeed
    # i2c adapters are described in device tree, and the bus number of
    # these i2c-muxes' channels are hardcoded (as below) to avoid
    # breaking the existing applications.
    #    i2c-mux 2-0070: child bus 16-23
    #    i2c-mux 8-0070: child bus 24-31
    #    i2c-mux 9-0070: child bus 32-39
    #    i2c-mux 11-0070: child bus 40-47
    last_child_bus=47

    # Create second-level i2c-muxes which are connected to first level
    # mux "8-0070". "8-0070" has 8 channels and the first 4 channels
    # are connected with 1 extra level of i2c-mux. So total 32 child
    # buses (48-79) will be registered.
    last_child_bus=$((last_child_bus + 8))
    i2c_mux_add_sync 24 0x71 ${i2c_mux_name} ${last_child_bus}

    last_child_bus=$((last_child_bus + 8))
    i2c_mux_add_sync 25 0x72 ${i2c_mux_name} ${last_child_bus}

    last_child_bus=$((last_child_bus + 8))
    i2c_mux_add_sync 26 0x76 ${i2c_mux_name} ${last_child_bus}

    last_child_bus=$((last_child_bus + 8))
    i2c_mux_add_sync 27 0x76 ${i2c_mux_name} ${last_child_bus}


    # Create second-level i2c-muxes which are connected to first level
    # mux "11-0070". "11-0070" has 8 channels and each channel is
    # connected with 1 extra level of i2c-mux (@ address 0x73), thus
    # totally 64 i2c buses (80-143) will be registered.
    parent_buses="40 41 42 43 44 45 46 47"
    for bus in ${parent_buses}; do
        last_child_bus=$((last_child_bus + 8))
        i2c_mux_add_sync ${bus} 0x73 ${i2c_mux_name} ${last_child_bus}
    done
}

KERNEL_VERSION=`uname -r`
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
    bulk_create_i2c_mux
fi

# # Bus 2
i2c_device_add 2 0x35 scmcpld          # SCMCPLD

# # Bus 12
i2c_device_add 12 0x3e smbcpld         # SMBCPLD

# # Bus 13
i2c_device_add 13 0x35 iobfpga         # IOBFPGA

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

if [ $board_rev -ne 4 ]; then
    # # Bus 55
    i2c_device_add 55 0x60 pdbcpld         # Left PDBCPLD

    # # Bus 63
    i2c_device_add 63 0x60 pdbcpld         # Right PDBCPLD
fi

# # Bus 49
i2c_device_add 49 0x59 psu_driver      # PDB-L PSU1 Driver

# # Bus 48
i2c_device_add 48 0x58 psu_driver      # PDB-L PSU2 Driver

# # Bus 57
i2c_device_add 57 0x59 psu_driver      # PDB-R PSU3 Driver

# # Bus 56
i2c_device_add 56 0x58 psu_driver      # PDB-R PSU4 Driver

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

# # Bus 86 94 102 110 118 126 134 142  # PIM1 ~ PIM8 MAX34461
BUS="86 94 102 110 118 126 134 142"
for bus in ${BUS}; do
    i2c_device_add $bus 0x74 max34461
done

# # Bus 80 88 96 104 112 120 128 136  # PIM1 ~ PIM8 DOMFPGA
BUS="80 88 96 104 112 120 128 136"
for bus in ${BUS}; do
    i2c_device_add $bus 0x60 domfpga
done
