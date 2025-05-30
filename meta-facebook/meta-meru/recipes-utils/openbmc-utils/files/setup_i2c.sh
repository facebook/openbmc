#!/bin/bash
#
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

if ! wedge_is_bmc_personality; then
    echo "uServer is not in BMC personality. Skipping all power-on sequences."
    exit 0
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

wait_sysfs_file_present() {
   sysfs_file="$1"
   timeout="$2"

   retries=$((timeout * 10))
   i=0
   while [ $i -lt $retries ]; do
      if [ -e "$sysfs_file" ]; then
         return
      fi
      sleep 0.1
      i=$((i + 1))
   done

   echo "sysfs_file $sysfs_file failed to appear after $timeout seconds"
}

i2c_device_add 0 0x50 24c512 # BMC EEPROM
i2c_device_add 9 0x50 24c512 # SCM EEPROM
i2c_device_add 9 0x23 smbcpld # SMBCPLD
i2c_device_add 9 0x52 24c512 # SMB EEPROM
i2c_device_add 12 0x43 pwrcpld

# Wait for the SMB EEPROM and CPLD drivers to load
modprobe at24
modprobe pwrcpld
wait_sysfs_file_present "$SMB_EEPROM_SYSFS" 10
wait_sysfs_file_present "$SCM_CPU_READY_SYSFS" 10

maybe_fix_dmi_config

userver_power_on

# Print total number of devices.
i2c_check_driver_binding
