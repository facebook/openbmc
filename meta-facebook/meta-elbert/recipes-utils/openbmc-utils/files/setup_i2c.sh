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

i2c_device_add 0 0x50 24c512
if ! wedge_is_bmc_personality; then
    echo "uServer is not in BMC personality. Skipping all power-on sequences."
    return
fi

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

# Calibrate TH4 MAX6581
calibrate_th4_max6581() {
    i2cset -f -y 4 0x4d 0x4a 0x03
    i2cset -f -y 4 0x4d 0x4b 0x1f
    i2cset -f -y 4 0x4d 0x4c 0x03
}

# Probe FSCD devices first in case one module is taking too
# long or is in bad shape
# Supvervisor Inlet
hwmon_device_add 11 0x4c max6658
hwmon_device_add 4 0x4d max6581
calibrate_th4_max6581

# SMBus 0
# Currently not using I2C TPM
# i2c_device_add 0 0x20 tpm_i2c_infineon
# i2c_device_add 0 0x2e tpm_i2c_infineon

# SMBus 3 - Switchcard VRD
hwmon_device_add 3 0x4e ucd90160 # UCD90160
i2c_device_add 3 0x60 raa228228  # RA228228
i2c_device_add 3 0x62 isl68226   # ISL68226
enable_switchcard_power_security

# SMBus 4
i2c_device_add 4 0x23 smbcpld
echo 0 > "$PIM_SMB_MUX_RST"
echo 0 > "$PSU_SMB_MUX_RST"
echo 1 > "$SMB_TH4_I2C_EN_SYSFS"
# Switchcard EEEPROMs
i2c_device_add 4 0x50 24c512
i2c_device_add 4 0x51 24c512
i2c_device_add 4 0x44 net_brcm

# SMBus 6
i2c_device_add 6 0x60 fancpld
# Outlet Temperature
i2c_device_add 6 0x4c max6658

# SMBus 7 CHASSIS EEPROM
i2c_device_add 7 0x50 24c512

# SMBus 9 SCM DPM UCD90320
retry_command 3 enable_power_security_mode 9 0x11 "UCD90320U"
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

# SMBus 2 PIM MUX, muxed as Bus 16-23
pim_index=(0 1 2 3 4 5 6 7)

pim_bus=(16 17 18 19 20 21 22 23)
if wedge_is_smb_p1; then
    # P1 has different PIM bus mapping
    pim_bus=(16 17 18 23 20 21 22 19)
    echo "P1 SMB detected"
fi

for i in "${pim_index[@]}"
do
    # PIM numbered 2-9
    pim=$((i+2))
    if wedge_is_pim_present "$pim"; then
        # PIM 2-9, SMBUS 16-23
        bus_id="${pim_bus[$i]}"
        i2c_device_add "$bus_id" 0x50 24c512  # EEPROM
        # Only allow writes to certain registers for
        # power controllers UCD9090/TPS546D24
        retry_command 3 enable_tps546d24_wp "$bus_id" 0x16
        retry_command 3 enable_tps546d24_wp "$bus_id" 0x18
        i2c_device_add "$bus_id" 0x16 pmbus   # TPS546D24
        i2c_device_add "$bus_id" 0x18 pmbus   # TPS546D24

        fru="$(peutil "$pim" 2>&1)"
        if echo "$fru" | grep -q '88-16CD2'; then
            echo "PIM$pim has no UCD9090/LM73"
        else
            retry_command 3 enable_power_security_mode "$bus_id" 0x4e "UCD9090B"
            i2c_device_add "$bus_id" 0x4a lm73    # Temp sensor
            i2c_device_add "$bus_id" 0x4e ucd9090 # UCD9090
        fi

        if echo "$fru" | grep -q '88-8D'; then
            i2c_device_add "$bus_id" 0x40 pmbus # MP5023
            i2c_device_add "$bus_id" 0x54 isl68224 # ISL68224
            retry_command 3 maybe_enable_isl_wp "$bus_id" 0x54
        fi
    else
        echo "PIM${pim} not present... skipping."
    fi
done

# SMBus 5 PSU MUX, muxed as Bus 24-27
for id in 1 2 3 4
do
    psu_prsnt="$(head -n 1 "$SMBCPLD_SYSFS_DIR"/psu"$id"_present)"
    if [ "$((psu_prsnt))" -eq 1 ]; then
        # PSU 1-4, SMBUS 22-25
        bus_id=$((23 + id))
        echo "Enable WRITE_PROTECT on PSU$id"
        retry_command 3 enable_psu_wp "$bus_id" 0x58
        i2c_device_add "$bus_id" 0x58 psu_driver
    else
        echo "PSU${id} not present... skipping."
    fi
done

# Print total number of devices.
i2c_check_driver_binding

# Apply lm_sensor threshold settings.
sensors -s
