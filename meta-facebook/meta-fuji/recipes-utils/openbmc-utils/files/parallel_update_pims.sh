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
#
# This tool is used to refresh dom fpga (parallel mode), after execution it will update all dom fpga firmware at the same time.
# The command format is
#
# parallel_update_pims.sh <dom fpga image>
#
# SC2009: Consider using pgrep instead of grepping ps output.
# because we dont have the pgrep, disable the SC2009 check
# shellcheck disable=SC2009

fpgaimg=$1
log_dir=/tmp/
tmpfile=/tmp/domfpga
DOM_FPGA_START=3

parallel_fpga_update(){
    # extend the image size to fit flash size
    cp "$fpgaimg" "$tmpfile"
    flashsize=$(flashrom -p linux_spi:dev=/dev/spidev$DOM_FPGA_START.0 | grep -i kB | xargs echo | cut -d '(' -f 2 | cut -d ' ' -f 0)
    filesize=$(stat -c%s $tmpfile)
    addsize=$(($((flashsize * 1024)) - filesize))

    if [ $((addsize)) -gt 0 ];then
        dd if=/dev/zero bs="$addsize" count=1 | tr "\000" "\377" >> "$tmpfile"
    fi

    # start dom fpga update
    printf "\nUsb-spi parallel dom fpga update:\n\n"
    for(( spidev=DOM_FPGA_START ; spidev<$((DOM_FPGA_START+8)) ; spidev++ ))
    do
        printf " \e[mstart pim %s dom fpga update.\e[m\n" "$((spidev-3))"
        ( flashrom -p linux_spi:dev=/dev/spidev"$spidev".0 -w "$tmpfile" > ${log_dir}flash_spidev${spidev}_multi_log )  &
    done
    printf "\n \e[mwaitting for the update finished...\e[m"
    while [ "$(ps w | grep -i flashrom | grep -v grep)" != "" ]
    do
        sleep 1
    done
    printf "\e[mdone.\e[m\n"
}

if [ "$1" != "" ] && [ -f "$1" ];then
    # switch pim mux from iob fpga to dom fpga
    switch_pim_mux_to_fpga.sh
    parallel_fpga_update
else
    echo "Usage:"
    echo "$0 <dom fpga image>"
fi
