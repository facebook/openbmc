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
PWRCPLD_SYSFS_DIR="/sys/bus/i2c/drivers/pwrcpld/12-0043"
SCM_PWR_IN_RESET_SYSFS="${PWRCPLD_SYSFS_DIR}/cpu_in_reset"
SCM_CPU_READY_SYSFS="${PWRCPLD_SYSFS_DIR}/cpu_ready"
SMB_EEPROM_SYSFS="/sys/bus/i2c/drivers/at24/9-0052/eeprom"

# SMB CPLD endpoints
SMBCPLD_SYSFS_DIR="/sys/bus/i2c/drivers/smbcpld/9-0023"
J3_ASIC_PCIE_RESET_SYSFS="${SMBCPLD_SYSFS_DIR}/j3_pcie_reset"
J3_ASIC_SYS_RESET_SYSFS="${SMBCPLD_SYSFS_DIR}/j3_system_reset"
R3_ASIC_0_ASIC_PCIE_RESET_SYSFS="${SMBCPLD_SYSFS_DIR}/ramon3_0_pcie_reset"
R3_ASIC_0_ASIC_SYS_RESET_SYSFS="${SMBCPLD_SYSFS_DIR}/ramon3_0_system_reset"
R3_ASIC_1_ASIC_PCIE_RESET_SYSFS="${SMBCPLD_SYSFS_DIR}/ramon3_1_pcie_reset"
R3_ASIC_1_ASIC_SYS_RESET_SYSFS="${SMBCPLD_SYSFS_DIR}/ramon3_1_system_reset"

LAYOUT_FILE="/etc/meru_flash.layout"

WEUTIL_CMD='weutil -e'

ACONF_SMB_SERIAL_CACHE="/mnt/data/.aconf_smb_serial"
TEMP_WEUTIL_OUTPUT="/tmp/tmp_weutil_output"
TEMP_ACONF_OUTPUT="/tmp/tmp_aconf_output"
ACONFUTIL_CMD='aconf_util.sh'

DS4520_BUS=8
DS4520_IO0_REG=0xf8

wedge_is_bmc_personality() {
    DEV_ADDR=""
    if i2cget -y 8 0x52 0x0 &> /dev/null; then
        DEV_ADDR=0x52
    else
        # P1 systems have DS4520 at 0x50
        echo "DS4520 not detected at 8-0052, trying 8-0050"
        DEV_ADDR=0x50
    fi

    io_reg_val=$(i2cget -y $DS4520_BUS $DEV_ADDR $DS4520_IO0_REG)
    if [ "$((io_reg_val & 0x1))" = "1" ]; then
        return 0
    else
        return 1
    fi
}

retry_command() {
    # Retry command up to $1 attempts
    local retries=$1
    shift

    local count=0
    until "$@"; do
        exit=$?
        count=$((count+1))
        if [ "$count" -lt "$retries" ]; then
            msg="Attempt $count/$retries failed with $exit, retrying..."
            echo "$msg"
            logger retry_command: "$msg"
        else
            msg="Retry $count/$retries failed with $exit, no more retries left"
            echo "$msg"
            logger retry_command: "$msg"
            return $exit
        fi
    done
    return 0
}

wedge_board_type() {
    echo 'meru'
}

wedge_product_name() {
    output=$($WEUTIL_CMD smb 2>&1) || { echo "$output"; return 1; }
    echo "$output" | awk -F': ' '/Product Name:/ {print $2}'
}

wedge_power_asic() {
    if [[ "$1" != "0" && "$1" != "1" ]]; then
        echo "Invalid arg: Please provide 0 for power on or 1 for power off."
        return 1
    fi

    if [ ! -e "$SMB_EEPROM_SYSFS" ]; then
        echo "Warn: SMB eeprom not detected; skipping asic power operation."
        return 1
    fi

    local power_state="$1"

    if ! product=$(wedge_product_name); then
        echo "Error: Failed to detect product name using weutil."
        echo "$product"
        return 1
    fi

    case "$product" in
        MERU800BIA|MERU800BIAB|MERU800BIAC)
            echo "$power_state" > "$J3_ASIC_PCIE_RESET_SYSFS"
            echo "$power_state" > "$J3_ASIC_SYS_RESET_SYSFS"
            ;;
        MERU800BFA)
            echo "$power_state" > "$R3_ASIC_0_ASIC_PCIE_RESET_SYSFS"
            echo "$power_state" > "$R3_ASIC_0_ASIC_SYS_RESET_SYSFS"
            echo "$power_state" > "$R3_ASIC_1_ASIC_PCIE_RESET_SYSFS"
            echo "$power_state" > "$R3_ASIC_1_ASIC_SYS_RESET_SYSFS"
            ;;
        *)
            echo "Error: Unexpected product name detected."
            return 1
            ;;
    esac
}

wedge_board_rev() {
    board_rev=$($WEUTIL_CMD scm|grep "Product Production State"|awk '{print $4}')
    case "$((board_rev))" in
        1)
            echo "EVT"
            ;;
        2)
            echo "DVT"
            ;;
        3)
            echo "PVT"
            ;;
        4)
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

wedge_is_scm_p1()
{
    board_rev=$(wedge_board_rev)
    if [ "${board_rev}" == "1" ]; then
        return 0
    fi

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

userver_power_on() {
    sync
    sleep 0.5
    # Power on using the SLG gpio
    i2cset -f -y 14 0x28 0x2e 0x1
    sleep 0.5
    wedge_power_asic 0
    return 0
}

userver_power_off() {
    # Power off using the SLG gpio
    i2cset -f -y 14 0x28 0x2e 0x0
    # Some delay is needed for "wedge_power reset" reset to take effect
    sleep 10
    return 0
}

userver_reset() {
    wedge_power_asic 1
    userver_power_off
    sleep 1
    userver_power_on
    return 0
}

chassis_power_cycle() {
    sleep 1
    i2cset -y -f 12 0x43 0x70 0xDE
    sleep 8
    # The chassis shall be reset now... if not, we are in trouble
    echo " Failed"
    return 254
}

bmc_mac_addr() {
    # get the MAC from Meru SCM EEPROM
    mac_base=$(userver_mac_addr)
    mac_base_hex=$(echo "$mac_base" |  tr '[:lower:]' '[:upper:]' | tr -d ':')
    mac_dec=$(printf '%d\n' 0x"$mac_base_hex")
    # Add 2 to SCM MAC base for BMC MAC
    mac_dec=$((mac_dec + 2))
    # Convert base to MAC format
    mac_hex=$(printf '%X\n' "$mac_dec")
    mac=$(echo "$mac_hex" | sed 's/../&:/g;s/:$//')
    echo "$mac"
}

# shellcheck disable=SC2120
userver_mac_addr() {
    # support v4 or v5 eeprom version
    $WEUTIL_CMD scm | grep -E '(Extended|CPU) MAC B' | awk -F': ' '{print $2}'
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
    # Loads spi-nor driver to channel
    # $1 - spi bus/channel. e.g: 1.0. or 1.1
    SPI="spi$1"
    SPIDEV="spidev$1"
    if [ -e /dev/"$SPIDEV" ]; then
        echo "$SPI" > /sys/bus/spi/drivers/spidev/unbind
    fi

    # Bind
    echo spi-nor  > /sys/bus/spi/devices/"$SPI"/driver_override
    if [ ! -e /sys/bus/spi/drivers/spi-nor/"$SPI" ]; then
        echo Binding "$SPI" to ...
        echo "$SPI" > /sys/bus/spi/drivers/spi-nor/bind
        sleep 0.5
    fi
    MTD="$(grep "$SPI" /proc/mtd |awk '{print$1}' | tr -d : | tr -d mtd)"
    if test -z "$MTD"; then
        echo "Failed to locate mtd partition for SPI Flash!"
        exit 1
    fi
}

unbind_spi_nor_driver() {
    # Dynamically Unload spi-nor driver form channel back to spidev
    # $1 - spi bus/channel. e.g: 1.0. or 1.1
    SPI="spi$1"
    SPIDEV="spidev$1"
    echo > /sys/bus/spi/devices/"$SPI"/driver_override
    if grep "$SPI" /proc/mtd > /dev/null 2>&1 ; then
        echo "Unbinding spi-nor from $SPI"
        echo "$SPI" > /sys/bus/spi/drivers/spi-nor/unbind
    fi
    if [ ! -e /dev/"$SPIDEV" ]; then
        echo "Binding spidev back to $SPI"
        echo "$SPI" > /sys/bus/spi/drivers/spidev/bind
    fi
}

get_section_start() {
    local name="$1"
    local startstr
    startstr=$(awk -v name="$name" '$2 == name {print $1}' $LAYOUT_FILE | \
               cut -d ':' -f1)
    echo "0x$startstr"
}

get_section_size() {
    local name="$1"
    local endstr
    local section_start
    local size

    section_start=$(get_section_start "$name")
    endstr=$(awk -v name="$name" '$2 == name {print $1}' $LAYOUT_FILE | \
             cut -d ':' -f2)
    section_end="0x$endstr"

    size=$((section_end + 1 - section_start))
    echo $size
}

get_total_size() {
    get_section_size total
}

do_spi_image() {
    TARGET_IMAGE="$1"   # Target Image path
    DRIVER="${2^^}"     # SPI driver used
    SPI_NAME="$3"       # SPI CHANNEL name: e.g 1.1
    SECTOR="${4^^}"     # SPI partition used
    PARTITION="$5"      # Layout file section
    ACTION="${6^^}"     # SPI action: e.g program/verify/read

    case "$DRIVER" in # Selects spi driver used
        FLASHROM)
           DRIVER=linux_spi:dev=/dev/spidev"$SPI_NAME"
           ;;
        SPINOR)
           SMB_MTD="$(grep spi"$SPI_NAME" /proc/mtd |awk '{print$1}' | tr -d : | tr -d mtd)"
           DRIVER=linux_mtd:dev="$SMB_MTD"
           ;;
        *)
           echo "Unknown driver $2 specified"
           exit 1
           ;;
    esac

    case "$SECTOR" in # Selects passed partition name
        BIOS)
           ;;
        *)
           echo "Unknown region $3 specified"
           exit 1
           ;;
    esac

    case "$ACTION" in
        READ|FULLREAD)
           OPERATION="-r"
           ;;
        PROGRAM|WRITE|FULLWRITE)
           OPERATION="-w"
           ;;
        VERIFY)
           OPERATION="-v"
           ;;
        ERASE)
           OPERATION="-E"
           ;;
        IDENTIFY)
           # try to match SPI chip string in output of flashrom
           # Use TARGET_IMAGE variable for parsing the output
           flashrom -p "$DRIVER" | grep -iq "$TARGET_IMAGE" || return 1
           return 0
           ;;
        *)
           echo "Unknown SPI ACTION $ACTION"
           exit 1
           ;;
    esac

    echo "Selected partition $PARTITION to $ACTION with layout file $LAYOUT_FILE"

    # If we are reading or erasing, simply do the operation and return
    if [ "$OPERATION" == "-r" ]; then
        flashrom -p "$DRIVER" --layout "$LAYOUT_FILE" --image "$PARTITION" \
           -r "$TARGET_IMAGE" || return 1
        sleep 1
        flashrom -p "$DRIVER" --layout "$LAYOUT_FILE" --image "$PARTITION" \
           -v "$TARGET_IMAGE" || return 1
        return 0
    elif [ "$OPERATION" == "-E" ]; then
        flashrom -p "$DRIVER" --layout "$LAYOUT_FILE" --image "$PARTITION" \
           -E || return 1
        return 0
    fi

    # Retry the programming if it fails up to 3 times
    n=1
    max=3
    while true; do
        # Program the spi image
        if flashrom -p "$DRIVER" -N --layout "$LAYOUT_FILE" \
           --image "$PARTITION" $OPERATION "$TARGET_IMAGE" ; then
            break
        else
            if [ $n -lt $max ]; then
                echo "Programming SPI failed, retry attempt $n/$max"
                n=$((n+1))
            else
                echo "Failed to program after $n attempts"
                exit 1
            fi
        fi
    done
}

maybe_fix_dmi_config() {
   $WEUTIL_CMD smb > "$TEMP_WEUTIL_OUTPUT" 2>&1
   serial_num=$(awk -F': ' '/Product Serial Number:/ {print $2}' "$TEMP_WEUTIL_OUTPUT")
   if [ -z "$serial_num" ]; then
      # weutil likely failed to read the serial number. May not yet be programmed.
      return
   fi

   if [ -f "$ACONF_SMB_SERIAL_CACHE" ]; then
      cache_serial=$(grep "$serial_num" "$ACONF_SMB_SERIAL_CACHE")
      if [ "$cache_serial" = "$serial_num" ]; then
         # No programming needed as the switch serial number is the same as the
         # cache file.
         return
      fi
   fi

   product=$(awk -F': ' '/Product Name:/ {print $2}' "$TEMP_WEUTIL_OUTPUT")
   if ! [[ "${product^^}" =~ "MERU" ]]; then
      echo "Not fixing aboot_conf DMI_BOARD_NAME as weutil product is not valid"
      return
   fi

   # Only check the DMI config on a cold boot so as not to bring down
   # the CPU.

   if userver_power_is_on; then
      return
   fi

   $ACONFUTIL_CMD show > "$TEMP_ACONF_OUTPUT"
   board_name=$(grep DMI_BOARD_NAME "$TEMP_ACONF_OUTPUT" | awk -F'=' '{print $2}')

   if [ "${board_name^^}" != "${product^^}" ]; then
      echo "DMI_BOARD_NAME $board_name does not match weutil product $product"
      echo "Fixing aboot_conf DMI_BOARD_NAME. Programming to $product"
      conf_type="cpu"
      if grep -q 57600 "$TEMP_ACONF_OUTPUT"; then
         conf_type="bmc"
      fi
      $ACONFUTIL_CMD -p "$product" program "$conf_type"
   fi

   # Record the serial number in a cache file to avoid checking on every
   # cold boot.
   echo "$serial_num" > "$ACONF_SMB_SERIAL_CACHE"
}
