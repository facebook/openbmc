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

# shellcheck disable=SC2034
DS4520_BUS=8
DS4520_IO0_REG=0xf8
PWRCPLD_SYSFS_DIR="/sys/bus/i2c/drivers/pwrcpld/12-0043"
SCM_CPU_READY_SYSFS="${PWRCPLD_SYSFS_DIR}/cpu_ready"
CPU_CONTROL_SYSFS="${PWRCPLD_SYSFS_DIR}/cpu_control"
SMB_NONSTDBY_PWR_SYSFS="${PWRCPLD_SYSFS_DIR}/smb_nonstdby_pwr"
CPU_NONSTDBY_PWR_SYSFS="${PWRCPLD_SYSFS_DIR}/cpu_nonstdby_pwr"
CPU_PWR_CYCLE_SYSFS="${PWRCPLD_SYSFS_DIR}/power_cycle"

WEUTIL_CMD='weutil -e'

wedge_is_cpu_personality() {
    io_reg_val=$(i2cget -f -y "$DS4520_BUS" 0x52 "$DS4520_IO0_REG")
    if [ "$((io_reg_val & 0x1))" = "1" ]; then
        return 0
    else
        return 1
    fi
}

wedge_board_type() {
    echo 'aristabmc'
}

wedge_board_rev() {
    # FIXME if needed.
    return 1
}

userver_power_is_on() {
    isCpuReady="$(head -n 1 "$SCM_CPU_READY_SYSFS" 2> /dev/null)"
    if [ "$isCpuReady" = "0x1" ]; then
        return 0 # uServer is on
    else
        return 1
    fi
}

userver_power_is_off() {
    ! userver_power_is_on
}

wait_until() {
    local deadline="$1" msg="$2"
    shift 2
    until "$@"; do
        if [ "$SECONDS" -ge "$deadline" ]; then
            echo "$msg"
            return 62  # ETIME
        fi
        sleep 1
    done
}

smb_power_on() {
    echo 1 > "$SMB_NONSTDBY_PWR_SYSFS"
    echo 1 > "$CPU_NONSTDBY_PWR_SYSFS"
}

userver_power_on() {
    sync
    sleep 0.5
    smb_power_on

    # Proceed with master core power ON unless user explicitly
    # requested not to at a bootloader stage.
    # Note: set bmc_only variable at uboot with:
    # setenv bootargs "${bootargs} bmc_only=yes"
    # saveenv
    if grep -q "bmc_only" /proc/cmdline; then
        echo "Skipping userver power on as bootargs set to BMC only mode."
        return 0
    fi

    # Power on using the cpld
    echo 1 > "$CPU_CONTROL_SYSFS"

    local deadline=$(( SECONDS + 15 ))
    wait_until "$deadline" "userver failed to power on" userver_power_is_on
}

userver_power_off() {
    # Power off using the cpld
    echo 0 > "$CPU_CONTROL_SYSFS"

    local deadline=$(( SECONDS + 15 ))
    wait_until "$deadline" "userver failed to power off" userver_power_is_off
}

userver_reset() {
    userver_power_off || return $?
    userver_power_on
}

chassis_power_cycle() {
    sleep 1
    echo 0xDE > "$CPU_PWR_CYCLE_SYSFS"
    sleep 8

    # The chassis shall be reset now... if not, we are in trouble
    echo " Failed"
    return 254
}

bmc_mac_addr() {
    local mac_offset

    mac_base=$(userver_mac_addr)
    mac_base_hex=$(echo "$mac_base" |  tr '[:lower:]' '[:upper:]' | tr -d ':')
    mac_dec=$(printf '%d\n' 0x"$mac_base_hex")
    mac_offset=3
    mac_dec=$((mac_dec + mac_offset))

    # Convert base to MAC format
    mac_hex=$(printf '%X\n' "$mac_dec")
    mac=$(echo "$mac_hex" | sed 's/../&:/g;s/:$//')
    echo "$mac"
}

# shellcheck disable=SC2120
userver_mac_addr() {
    local eeprom_source
    eeprom_source="scm"
    # support v4 or v5/v6 eeprom version
    $WEUTIL_CMD "$eeprom_source" | grep -E '(Extended|CPU) MAC B' | awk -F': ' '{print $2}'
}

FWUPGRADE_PIDFILE="/var/run/firmware_upgrade.pid"
check_fwupgrade_running()
{
    exec 200>$FWUPGRADE_PIDFILE
    flock -n 200 || (echo "Another FW upgrade is running" && exit 1)
    ret=$?
    if [ $ret -eq 1 ]; then
      exit 1
    fi
    pid=$$
    echo $pid 1>&200
}
