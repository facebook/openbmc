#!/bin/bash
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
# This tool is used to update dom fpga (usb-spi mode)
# The command format is
#
# Command to upgrade all pims in parallel:
#  pim_upgrade.sh all <dom fpga image>
#
# Command to upgrade a single pim:
#  pim_upgrade.sh pim1/pim2/pim3/pim4/pim5/pim6/pim7/pim8 <dom fpga image>
#
# SC2009: Consider using pgrep instead of grepping ps output.
# because we dont have the pgrep, disable the SC2009 check
# shellcheck disable=SC2009
# shellcheck disable=SC1091
. /usr/local/bin/flashrom-utils.sh

target=$1
fpgaimg=$2
log_dir=/tmp/
tmpfile=/tmp/domfpga
MAX_PIMNUM=8

get_pim_spidev() {
    pim_id="$1"
    pim_devmap_dir=/run/devmap/spi-devices
    spidev_path=$(readlink "${pim_devmap_dir}/PIM${pim_id}_FLASH")
    spidev_file=$(basename "$spidev_path")
    echo "$spidev_file"
}

flash_chip_expected_list() {
    # Micron : MX25U3232FM2I02 , MX25U3235FM2I-10G
    echo "MX25U3235E/F"
    # Winbond : W25Q32JWSSIQ
    echo "W25Q32JW...Q"
}

# Override using flash_extrach_info from flashrom-utils.sh
#
# helper to fix the issue flashrom detect multiple flash chips
# - "MX25L3239E"      failed one
# - "MX25U3235E/F"    good one
#
# by compare detected flash chips with expected good flash chip list
#
get_flash_info() {
    pim_flash="$1"
    valid=0
    # replace space with underscore for easier parsing
    info=$(flash_extract_info "$pim_flash" | tr ' ' '+')
    # Validate flash info amount
    count=$(echo "$info" | wc -l)

    if [ $((count)) -eq 1 ]; then
        # In case only one flash chip detected, just return it
        echo "$info"
    elif [ $((count)) -gt 1 ]; then
        expected_list=$(flash_chip_expected_list)
        # get the first detected from valid chip list
        for chip in $expected_list; do
            # split by line and check each line
            for line in $info; do
                if echo "$line" | grep -q "$chip"; then
                    echo "$line" | tr '+' ' '
                    valid=1
                    break 2
                fi
            done
        done
        if [ $((valid)) -ne 1 ]; then
            echo "None of the detected flash chips are in good list!" >&2
            echo " $expected_list" >&2
            flash_dump_summary "$1"
            return 1
        fi
    else
        echo "No flash chip detected! flashrom output:"
        flash_dump_summary "$1"
        return 1
    fi
}

get_flash_model() {
    pim_flash="$1"
    info=$(get_flash_info "$pim_flash")
    model=$(echo "$info" | cut -d '"' -f 2)
    if [ -z "$model" ]; then
        echo "Unable to determine flash model! flashrom output:"
        flash_dump_summary "$1"
        return 1
    fi
    echo "$model"
}

get_flash_size() {
    pim_flash="$1"
    info=$(get_flash_info "$pim_flash")
    size=$(echo "$info" | cut -d '(' -f 2 | cut -d ' ' -f 1)
    if ! is_decimal "$size"; then
        echo "Unable to determine flash size! flashrom output:"
        flash_dump_summary "$1"
        return 1
    fi
    echo "$size"
}

fpga_update(){
    # extend the image size to fit flash size
    cp "$fpgaimg" "$tmpfile"
    filesize=$(stat -c%s $tmpfile)

    # start dom fpga update
    printf "\nUsb-spi dom fpga update:\n\n"

    pim1_flash=$(get_pim_spidev 1)
    flash_size=$(get_flash_size "$pim1_flash")
    addsize=$(($((flash_size * 1024)) - filesize))

    if [ $((addsize)) -gt 0 ];then
        dd if=/dev/zero bs="$addsize" count=1 | tr "\000" "\377" >> "$tmpfile"
    fi

    if [ "$target" == "all" ];then

        for(( pimnum=1 ; pimnum<=MAX_PIMNUM ; pimnum++ ))
        do
        printf " \e[mstart pim %s dom fpga update.\e[m\n" "$((pimnum))"
        pim_flash=$(get_pim_spidev "$pimnum")
        flash_model=$(get_flash_model "$pim_flash")
        ( flash_write "$pim_flash" "$tmpfile" "$flash_model" > ${log_dir}flash_"${pim_flash}"_multi_log )  &
        done
    else
    pimnum=$(($(echo "$target" | cut -b 4)))
    printf " \e[mstart pim %s dom fpga update.\e[m\n" "$((pimnum))"

    pim_flash=$(get_pim_spidev "$pimnum")
    flash_model=$(get_flash_model "$pim_flash")
    ( flash_write "$pim_flash" "$tmpfile" "$flash_model" > ${log_dir}flash_"${pim_flash}"_single_log )  &
    fi

    printf "\n \e[mwaitting for the update finished...\e[m"

    while [ "$(ps w | grep -i flashrom | grep -v grep)" != "" ]
    do
        sleep 1
    done

    printf "\e[mdone.\e[m\n"
}

case $target in
    "pim1" | "pim2" | "pim3" | "pim4" | "pim5" | "pim6" | "pim7" | "pim8" | "all")
        if [ "$fpgaimg" != "" ] && [ -f "$fpgaimg" ];then
            # switch pim mux from dom fpga to dom fpga
            switch_pim_mux_to_fpga.sh
            fpga_update
	    exit 0
        fi
    ;;
    *)
    ;;
esac

echo "Usage:"
echo "Command to upgrade all pims in parallel:"
echo "  $0 all <dom fpga image>"
echo ""
echo "Command to upgrade a single pim:"
echo "  $0 pim1/pim2/pim3/pim4/pim5/pim6/pim7/pim8 <dom fpga image>"

