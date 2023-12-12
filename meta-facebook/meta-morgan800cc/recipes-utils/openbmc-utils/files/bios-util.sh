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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

select_bios_spi_flash() {
    gpio_set_value BMC_SPI1_MUX 1
}

unselect_bios_spi_flash() {
    gpio_set_value BMC_SPI1_MUX 0
}

create_mtd_partition() {
    mtd_part=$(grep "bios" /proc/mtd | awk -F'[: ]' '{print $1}')

    # Return if already created
    if [ "" != "${mtd_part}" ]; then
        return
    fi

    if [ -w /sys/bus/platform/drivers/spi-aspeed-smc/1e630000.spi ]; then
        echo 1e630000.spi > /sys/bus/platform/drivers/spi-aspeed-smc/unbind
    fi
    echo 1e630000.spi > /sys/bus/platform/drivers/spi-aspeed-smc/bind
}

delete_mtd_partition() {
    if [ -w /sys/bus/platform/drivers/spi-aspeed-smc/1e630000.spi ]; then
        echo 1e630000.spi > /sys/bus/platform/drivers/spi-aspeed-smc/unbind
    fi
}

check_x86_power() {
    if userver_power_is_on; then
        echo "BIOS SPI flash upgrade requires x86 in power off state. Skipping upgrade!!"
        unselect_bios_spi_flash
        delete_mtd_partition
        exit 1
    fi
}

upgrade_bios_mtd_partition() {
    mtd_part=$(grep "bios" /proc/mtd | awk -F'[: ]' '{print $1}')
    if ! sudo flashcp -v "$upgrade_file" "/dev/$mtd_part"; then
        echo "Programming microinit image FAILED"
        unselect_bios_spi_flash
        delete_mtd_partition
        exit 1
    fi
}

upgrade_bios() {
    # Check x86 power and exit if already on
    check_x86_power

    # Setup SPI mux for BMC access
    select_bios_spi_flash

    # Check if mtd partition exist and create if not present.
    create_mtd_partition

    # flashcp command and continue if succeeded
    upgrade_bios_mtd_partition

    # Setup SPI mux for x86 access
    unselect_bios_spi_flash

    # Delete mtd parition after recovery
    delete_mtd_partition
}

print_bios_verison() {

    # Configure mux and mount partition
    select_bios_spi_flash
    create_mtd_partition

    # Retrieve the version
    tmp_file="/tmp/tmp_bios_version"
    dd bs=1 skip=61075470 count=16 if="/dev/mtd7" of="$tmp_file"
    if [ -s $tmp_file ]; then
        bios_version=$(tr -d '\0' < "$tmp_file")
        #rm "$tmp_file"
    else
        echo "Unable to retrieve vesion from flash"
    fi

    # Reconfigure mux and delete partition
    unselect_bios_spi_flash
    delete_mtd_partition

    # Print the BIOS version
    if [ -n "$bios_version" ]; then
        echo "BIOS Version : ${bios_version}"
    fi
}

bios_flash_helper() {
    if [ "$1" = "upgrade" ]; then
        if [ -f "$2" ]; then
            upgrade_file=$2
            upgrade_bios
        else
            echo "File $2 is empty or does not exist"
        fi
    fi

    if [ "$1" = "version" ]; then
        print_bios_verison
    fi
}

case "$1" in
    "upgrade" | "version" )
        bios_flash_helper "$1" "$2"
    ;;

    *)
        program=$(basename "$0")
        echo "Usage: BIOS flash utility"
        echo "  <$program upgrade image> enable access to BIOS flash, upgrade flash with provided image, then disable access"
        echo "  <$program version> get the programmed version from BIOS flash"
    ;;
esac

