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

LAYOUT_FILE="/etc/aristabmc_cpu_flash.layout"

wedge_is_cpu_personality() {
    io_reg_val=$(i2cget -f -y "$DS4520_BUS" 0x52 "$DS4520_IO0_REG")
    if [ "$((io_reg_val & 0x1))" = "1" ]; then
        return 0
    else
        return 1
    fi
}

wedge_board_type() {
    echo "aristabmc"
}

wedge_product_eeprom_source() {
    echo "chassis_eeprom"
}

wedge_product_name() {
    local eeprom_source output
    eeprom_source=$(wedge_product_eeprom_source)
    output=$($WEUTIL_CMD "$eeprom_source" 2>/dev/null) || return 1
    echo "$output" | awk -F ': ' '/^Product Name:/ { print $2; exit }'
}

wedge_board_rev() {
    local eeprom_source
    eeprom_source=$(wedge_product_eeprom_source)
    board_rev=$($WEUTIL_CMD "$eeprom_source"|grep "Production State"|awk -F':' '{print $2}'|xargs)
    case "$board_rev" in
        1|"EVT")
            echo "EVT"
            ;;
        2|"DVT")
            echo "DVT"
            ;;
        3|"PVT")
            echo "PVT"
            ;;
        4|"MP")
            echo "MP"
            ;;
        *)
            echo "Revision: unknown value [$board_rev]"
            ;;
    esac
}

wedge_board_type_rev() {
    board_type=$(wedge_board_type)
    board_rev=$(wedge_board_rev)

    if [ -z "$board_type" ] || [ -z "$board_rev" ]; then
        echo "Error: Unable to determine board type or revision!"
        return 1
    fi

    echo "${board_type}_${board_rev}"
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
    eeprom_source=$(wedge_product_eeprom_source)
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

bind_spi_nor_driver() {
    # Ensure SPI bus/chip-select $1 is bound to spi-nor.
    local spi="spi${1:?}"
    local driver

    driver=$(basename "$(readlink "/sys/bus/spi/devices/$spi/driver")" \
        2>/dev/null) || driver=""
    if [ "$driver" = spidev ]; then
        echo "Unbinding $spi from spidev"
        echo "$spi" > /sys/bus/spi/drivers/spidev/unbind || return 1
        sleep 0.5
    elif [ -n "$driver" ] && [ "$driver" != spi-nor ]; then
        echo "$spi is unexpectedly bound to $driver" >&2
        return 1
    fi

    # A boot-time probe occurs before the external CPU-flash mux is selected.
    # The spi-nor driver can remain bound even though that probe found no chip
    # and therefore created no MTD. Force a fresh probe after selecting the
    # mux rather than treating the stale driver binding as success.
    if [ "$driver" = spi-nor ] && ! spi_mtd_for_channel "$1" > /dev/null; then
        echo "Rebinding $spi to spi-nor after its boot-time probe failed"
        echo "$spi" > /sys/bus/spi/drivers/spi-nor/unbind || return 1
        driver=""
        sleep 0.5
    fi

    if [ "$driver" != spi-nor ]; then
        echo "Binding $spi to spi-nor"
        echo "$spi" > /sys/bus/spi/drivers/spi-nor/bind || return 1
        sleep 0.5
    fi

    if ! spi_mtd_for_channel "$1" > /dev/null; then
        echo "Failed to locate the MTD device for $spi" >&2
        return 1
    fi
}

unbind_spi_nor_driver() {
    # Remove the temporary MTD before disconnecting the external flash mux.
    local spi="spi${1:?}"
    local driver

    driver=$(basename "$(readlink "/sys/bus/spi/devices/$spi/driver")" \
        2>/dev/null) || driver=""
    if [ "$driver" = spi-nor ]; then
        echo "Unbinding $spi from spi-nor"
        echo "$spi" > /sys/bus/spi/drivers/spi-nor/unbind || return 1
        sleep 0.5
    fi
}

spi_mtd_for_channel() {
    # Return the MTD device dynamically created for SPI bus/chip-select $1.
    local matches=(/sys/bus/spi/devices/spi"${1:?}"/mtd/mtd[0-9]*)
    local mtd

    if [ "${#matches[@]}" -eq 1 ] && [ -e "${matches[0]}" ]; then
        basename "${matches[0]}"
        return
    fi

    # Linux 6.18 does not expose this SPI-NOR MTD below the SPI device's
    # sysfs directory. Aristabmc supplies its unique label for /proc/mtd.
    if [ -n "${SPI_MTD_LABEL:-}" ]; then
        mtd=$(mtd_lookup_by_name "$SPI_MTD_LABEL")
        if [ -n "$mtd" ]; then
            echo "$mtd"
            return
        fi
    fi

    return 1
}

do_spi_image() {
    # $1 image, $2 FLASHROM|SPINOR, $3 bus/chip-select, $4 BIOS,
    # $5 operation, remaining arguments are flash-layout partitions.
    local image="$1" driver="$2" spi_name="$3" region="$4" action="$5"
    local mtd layout operation
    local -a partitions partition_args
    shift 5
    partitions=("$@")

    case "${driver^^}" in
        FLASHROM) driver="linux_spi:dev=/dev/spidev${spi_name}" ;;
        SPINOR)
            mtd=$(spi_mtd_for_channel "$spi_name") || {
                echo "Failed to locate the MTD device for spi${spi_name}" >&2
                return 1
            }
            # flashrom expects the numeric MTD index (for example "7"), not
            # the kernel device name ("mtd7").
            driver="linux_mtd:dev=${mtd#mtd}"
            ;;
        *) echo "Unknown SPI driver: $driver" >&2; return 1 ;;
    esac

    [ "${region^^}" = BIOS ] || { echo "Unknown SPI region: $region" >&2; return 1; }
    case "${action^^}" in
        READ|FULLREAD) operation=-r ;;
        WRITE|PROGRAM|FULLWRITE) operation=-w ;;
        VERIFY) operation=-v ;;
        ERASE) operation=-E ;;
        *) echo "Unknown SPI operation: $action" >&2; return 1 ;;
    esac

    layout="$LAYOUT_FILE"
    for partition in "${partitions[@]}"; do
        partition_args+=(--include "$partition")
    done

    case "$operation" in
        -r)
            flashrom -p "$driver" --layout "$layout" "${partition_args[@]}" -r "$image" || return 1
            sleep 1
            flashrom -p "$driver" --layout "$layout" "${partition_args[@]}" -v "$image"
            ;;
        -w)
            local attempt=1
            while ! flashrom -p "$driver" -N --layout "$layout" "${partition_args[@]}" -w "$image"; do
                [ "$attempt" -lt 3 ] || return 1
                echo "Programming SPI failed; retrying ($attempt/3)"
                attempt=$((attempt + 1))
            done
            ;;
        -E) flashrom -p "$driver" --layout "$layout" "${partition_args[@]}" -E ;;
        -v) flashrom -p "$driver" --layout "$layout" "${partition_args[@]}" -v "$image" ;;
    esac
}