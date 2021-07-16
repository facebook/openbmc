#!/bin/bash
#
# Copyright 2020-present Delta Eletronics, Inc. All Rights Reserved.
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

source /usr/local/bin/openbmc-utils.sh

prog="$0"
img="$1"

DLL_PATH=/usr/lib/libcpldupdate_dll_gpio.so


usage() {
    echo "Usage: $prog <img_file> <options: hw|sw>"
    echo
    echo "img_file: Image file for lattice CPLD"
    echo "  VME file for software mode"
    echo "  JED file for hardware mode"
    echo "options:"
    echo "  hw: Program the CPLD using JTAG hardware mode"
    echo "  sw: Program the CPLD using JTAG software mode"
    echo
    echo
}

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

enable_jtag_chain(){
    gpio_set BMC_CPLD_UPDATE_EN        1
}

disable_jtag_chain(){
    gpio_set BMC_CPLD_UPDATE_EN        0
}

trap 'rm -rf /tmp/smbcpld_update' INT TERM QUIT EXIT

echo 1 > /tmp/smbcpld_update

delta_prepare_cpld_update

enable_jtag_chain

case $2 in
    hw)
        cpldprog -p "${img}"
        ;;
    sw)
        ispvm -f 1000 dll $DLL_PATH "${img}" \
        --tdi BMC_TDI \
        --tdo BMC_TDO \
        --tms BMC_TMS \
        --tck BMC_TCK
        ;;
    *)
        # default: hw mode
        cpldprog -p "${img}"
        ;;
esac

result=$?

if [ "$2" = "sw" ]; then
    expect=1
else
    expect=0
fi

disable_jtag_chain

# 0 is returned upon upgrade success
if [ $result -eq $expect ]; then
    echo "Upgrade successful."
    echo "Re-start fscd service."
    sv start fscd
    exit 0
else
    echo "Upgrade failure. Return code from utility : $result"
    echo "To prevent system reboot. Keep fscd stop & watchdog disable."
    exit 1
fi

