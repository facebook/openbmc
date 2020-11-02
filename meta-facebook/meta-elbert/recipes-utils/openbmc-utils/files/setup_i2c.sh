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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# Some devices occasionally fail to probe. Retry if this happens.
hwmon_device_add() {
    bus="$1"
    devIdHex="$2"
    driver="$3"
    devId="${devIdHex//0x/}"
    i2c_device_add "$bus" "$devIdHex" "$driver"

    # Wait up to 2 seconds for the driver to load
    i=0
    while true; do
        i=$((i+1))
        # Check if device driver loaded successfully
        if [ -d "/sys/bus/i2c/devices/$bus-00$devId/hwmon/" ]; then
            break;
        fi
        if [ $i -gt 20 ]; then
            echo "Timed out waiting for $driver-$bus-$devId"
            break;
        fi
        sleep 0.1
    done

    # If device isn't loaded sucessfully, retry one more time
    if [ ! -d "/sys/bus/i2c/devices/$bus-00$devId/hwmon/" ]; then
        echo "Probe of $driver-$bus-$devId failed. Retrying..."
        i2c_device_delete "$bus" "$devIdHex"
        i2c_device_add "$bus" "$devIdHex" "$driver"
    fi
}

# Probe FSCD devices first in case one module is taking too
# long or is in bad shape
# Supvervisor Inlet
hwmon_device_add 11 0x4c max6658

# SMBus 0
# Currently not using I2C TPM
# i2c_device_add 0 0x20 tpm_i2c_infineon
# i2c_device_add 0 0x2e tpm_i2c_infineon
i2c_device_add 0 0x50 24c512

# SMBus 3 - Switchcard ECB/VRD
i2c_device_add 3 0x40 pmbus
i2c_device_add 3 0x41 pmbus
hwmon_device_add 3 0x4e ucd90160 # UCD90160
i2c_device_add 3 0x60 isl68137   # RA228228
i2c_device_add 3 0x62 isl68137   # ISL68226

# SMBus 4
i2c_device_add 4 0x23 smbcpld
# Switchcard EEEPROMs
i2c_device_add 4 0x50 24c512
i2c_device_add 4 0x51 24c512
echo 0 > "$PIM_SMB_MUX_RST"
echo 0 > "$PSU_SMB_MUX_RST"

# SMBus 6
i2c_device_add 6 0x60 fancpld
# Outlet Temperature
i2c_device_add 6 0x4c max6658

# SMBus 7 CHASSIS EEPROM
i2c_device_add 7 0x50 24c512

# SMBus 9 SCM DPM UCD90320
hwmon_device_add 9 0x11 ucd90320
gpio_export_by_offset 9-0011 13 SCM_FPGA_LATCH_L

# SMBus 10 SCM POWER
i2c_device_add 10 0x30 cpupwr
i2c_device_add 10 0x37 mempwr

# SMBus 11 SCM TEMP, POWER
i2c_device_add 11 0x40 pmbus

# Bus  12 SCM CPLD, SCM EEPROM
i2c_device_add 12 0x43 scmcpld
i2c_device_add 12 0x50 24c512

# SMBus 15
hwmon_device_add 15 0x4a lm73
i2c_device_add 15 0x43 pfrcpld

# SMBus 2 PIM MUX, muxed as Bus 16-23
pim_index=(0 1 2 3 4 5 6 7)
pim_bus=(16 17 18 23 20 21 22 19)
for i in "${pim_index[@]}"
do
    # PIM numbered 2-9
    pim=$((i+2))
    pim_prsnt="$(head -n 1 "$SMBCPLD_SYSFS_DIR"/pim"$pim"_present)"
    if [ "$((pim_prsnt))" -eq 1 ]; then
        # PIM 2-9, SMBUS 16-23
        bus_id="${pim_bus[$i]}"
        i2c_device_add "$bus_id" 0x16 pmbus   # TPS546D24
        i2c_device_add "$bus_id" 0x18 pmbus   # TPS546D24
        i2c_device_add "$bus_id" 0x4a lm73    # Temp sensor
        i2c_device_add "$bus_id" 0x4e ucd9090 # UCD9090A
        i2c_device_add "$bus_id" 0x50 24c512  # EEPROM
    else
        echo "PIM${pim} not present... skipping."
    fi
done

# SMBus 5 PSU MUX, muxed as Bus 24-27
# ELBERTTODO Do we need to enable WP for PSU?
for id in 1 2 3 4
do
    psu_prsnt="$(head -n 1 "$SMBCPLD_SYSFS_DIR"/psu"$id"_present)"
    if [ "$((psu_prsnt))" -eq 1 ]; then
        # PSU 1-4, SMBUS 22-25
        bus_id=$((23 + id))
        i2c_device_add "$bus_id" 0x58 pmbus
    else
        echo "PSU${id} not present... skipping."
    fi
done
