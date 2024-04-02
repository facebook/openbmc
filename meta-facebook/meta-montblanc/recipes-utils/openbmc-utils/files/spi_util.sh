#!/bin/sh
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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
#

#shellcheck disable=SC1091
#shellcheck disable=SC2086
. /usr/local/bin/openbmc-utils.sh

SPI1_MASTER="1e630000.spi"
ASPEED_SMC_DRIVER_DIR="/sys/bus/platform/drivers/spi-aspeed-smc"

#
# Both come_cpld and scm_cpld are needed to select/deselect chips.
#
COMECPLD_I2C_BUS=0
COMECPLD_I2C_ADDR=0x1f
SCMCPLD_I2C_BUS=1
SCMCPLD_I2C_ADDR=0x35

#
# The chipselect and mtd labels assigned to IOB and BIOS.
#
IOB_MTD_LABEL="host-flash"
BIOS_MTD_LABEL="host-flash"

PID_FILE="/var/run/spi_util.pid"

cleanup_spi() {
    echo "Restore environment.."
    # select muxing FLASH to normal mode
    # BMC GPIO: IOB_FLASH
    #              to (X) IOB_FPGA    0
    #                 ( ) BMC_SPI1    1
    gpiocli -s IOB_FLASH_SEL set-value 0
    # BMC GPIO: BIOS_FLASH
    #               to (X) NetLake CPU 0
    #                  ( ) BMC_SPI1    1
    gpiocli -s SPI_MUX_SEL set-value 0
    # COME CPLD: BIOS_FLASH_CS0
    #               to ( ) BMC_SPI1_CS1 0x7
    #                  ( ) BMC_SPI1_CS0 0x5
    #                  (X) SPI_PCH_CS0 0x4
    #                  ( ) SPI_PCH_CS1 0x6
    i2cset -f -y "$COMECPLD_I2C_BUS" "$COMECPLD_I2C_ADDR" 0xa 0x4

    rm -rf /tmp/.spi_util*
    rm -f "$PID_FILE"
}

check_duplicate_process() {
    exec 2>"$PID_FILE"
    if ! flock -n 2; then
        echo "Another instance is running. Exiting!!"
        exit 1
    fi
}

enable_cpld_access() {
    echo "Enable access of COME and SCM CPLDs.."

    gpiocli -s BMC_I2C1_EN set-value 1
    gpiocli -s BMC_I2C2_EN set-value 1
}

select_iob_flash() {
    echo "Select IOB_FPGA flash.."

    # SCM CPLD: BMC_SPI1
    #              to (X) IOB_FPGA   0
    #                 ( ) COME_BIOS  1
    #                 ( ) PRoT       2
    i2cset -f -y "$SCMCPLD_I2C_BUS" "$SCMCPLD_I2C_ADDR" 0x34 0x0

    # BMC GPIO: IOB_FLASH
    #              to ( ) IOB_FPGA 0
    #                 (X) BMC_SPI1 1
    gpiocli -s IOB_FLASH_SEL set-value 1
}

select_bios_flash() {
    echo "Select BIOS flash.."

    # SCM CPLD: BMC_SPI1
    #              to ( ) IOB_FPGA   0
    #                 (X) COME_BIOS  1
    #                 ( ) PRoT       2
    i2cset -f -y "$SCMCPLD_I2C_BUS" "$SCMCPLD_I2C_ADDR" 0x34 0x1

    # BMC GPIO: BIOS_FLASH
    #              to ( ) NetLake CPU 0
    #                 (X) BMC_SPI1    1
    gpiocli -s SPI_MUX_SEL set-value 1

    # COME CPLD: BIOS_FLASH_CS0
    #               to ( ) BMC_SPI1_CS1 0x7
    #                  (X) BMC_SPI1_CS0 0x5
    #                  ( ) SPI_PCH_CS0 0x4
    #                  ( ) SPI_PCH_CS1 0x6
    i2cset -f -y "$COMECPLD_I2C_BUS" "$COMECPLD_I2C_ADDR" 0xa 0x5
}

probe_flashes() {
    echo "Probe flashes.."

    echo "$SPI1_MASTER" > "${ASPEED_SMC_DRIVER_DIR}/unbind"
    sleep 1

    echo "$SPI1_MASTER" > "${ASPEED_SMC_DRIVER_DIR}/bind"
    sleep 1
}

flash_mtd_read() {
    mtd_idx="$1"
    file="$2"

    echo "Read /dev/mtd${mtd_idx} to $file..."
    flashrom -p linux_mtd:dev="$mtd_idx" -r "$file"
}

flash_mtd_erase() {
    mtd_idx="$1"

    echo "Erase /dev/mtd$mtd_idx.."
    flashrom -p linux_mtd:dev="$mtd_idx" -E
}

expand_image_file() {
    mtd_idx=$1
    file=$2

    flashsize=$(flashrom -p linux_mtd:dev=$mtd_idx | grep -i kB |
                xargs echo | cut -d '(' -f 2 | cut -d ' ' -f 0)
    targetsize=$((flashsize * 1024))
    filesize=$(stat -c%s $file)
    add_size=$((targetsize - filesize))

    if [ "$add_size" -gt 0 ]; then
        echo "Append $add_size bytes to $file.."
        dd if=/dev/zero bs="$add_size" count=1 | tr "\000" "\377" >> "$file"
    fi
}

flash_mtd_write() {
    mtd_idx=$1
    file=$2
    tmpfile="/tmp/.spi_util_image.tmp"

    echo "Copy $file to $tmpfile.."
    cp "$file" "$tmpfile"

    expand_image_file "$mtd_idx" "$tmpfile"

    echo "Write $tmpfile to /dev/mtd$mtd_idx.."
    flashrom -p linux_mtd:dev="$mtd_idx" -w "$tmpfile"
}

do_flash_io() {
    action=$1
    device=$2
    file=$3
    mtd_label=""

    enable_cpld_access

    # Select the right flash chip
    case "$device" in
        "IOB_FPGA")
            select_iob_flash
            mtd_label="${IOB_MTD_LABEL}"
        ;;

        "COME_BIOS")
            select_bios_flash
            mtd_label="${BIOS_MTD_LABEL}"
        ;;

        *)
            echo "Error: unsupported device <$device>!"
            exit 1
        ;;
    esac

    # Force re-probe flashes
    probe_flashes
    mtd=$(mtd_lookup_by_name "$mtd_label")
    mtd_idx=$(echo "$mtd" | sed -e 's/mtd//')
    if [ -z "$mtd_idx" ]; then
        echo "Error: unable to find <$mtd_label> mtd partition!"
        exit 1
    fi

    # Read/write/erase the flash chip
    case "$action" in
        "read")
            if [ -z "$file" ]; then
                echo "Error: <pathname> is mandatory for $action!"
                exit 1
            fi
            flash_mtd_read "$mtd_idx" "$file"
        ;;

        "write")
            if [ ! -e "$file" ]; then
                echo "Error: <$file> does not exist!"
                exit 1
            fi
            flash_mtd_write "$mtd_idx" "$file"
        ;;

        "erase")
            flash_mtd_erase "$mtd_idx"
        ;;

        *)
            echo "Error: operation <$action> is not supported!"
            exit 1
        ;;
    esac
}

usage() {
    program="$1"

    echo "Usage:"
    echo "  $program <op> <spi> <spi device> <file>"
    echo "    <op>          : read, write, erase"
    echo "    <spi>         : spi1"
    echo "    <spi1 device> : IOB_FPGA, COME_BIOS"
    echo ""
    echo "Examples:"
    echo "  $program read spi1 IOB_FPGA iofpga.bit"
    echo ""
}

#
# Main entry starts here
#
check_duplicate_process

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    usage "$0"
    exit 0
fi

if [ "$#" -ne 4 ] && [ "$#" -ne 5 ]; then
    echo "Error: invalid command line arguments!"
    usage "$0"
    exit 1
fi

action="$1"
spi_bus="$2"
device="$3"
image_file="$4"

if [ "$spi_bus" != "spi1" ]; then
    echo "Error: invalid spi bus: $spi_bus!"
    exit 1
fi

trap cleanup_spi INT TERM QUIT EXIT

do_flash_io "$action" "$device" "$image_file"
