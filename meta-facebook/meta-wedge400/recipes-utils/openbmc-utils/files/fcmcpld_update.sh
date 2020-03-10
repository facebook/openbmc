#!/bin/bash
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

DLL_PATH=/usr/lib/libcpldupdate_dll_ast_jtag.so


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
    gpio_set BMC_JTAG_MUX_IN        1
    gpio_set PWR_CPLD_JTAG_EN_N     1
    gpio_set SYS_CPLD_JTAG_EN_N     0
    gpio_set SCM_CPLD_JTAG_EN_N     1
    gpio_set FCM_CPLD_JTAG_EN_N     1
    gpio_set PWR_CPLD_HITLESS       0
    gpio_set FCM_CPLD_HITLESS       1
    gpio_set SCM_CPLD_HITLESS       0
    gpio_set SMB_CPLD_HITLESS       0
    gpio_set BMC_FCM_SEL            0
    gpio_set BMC_SCM_CPLD_EN        1
}

disable_jtag_chain(){
    gpio_set BMC_JTAG_MUX_IN        0
    gpio_set PWR_CPLD_JTAG_EN_N     1
    gpio_set SYS_CPLD_JTAG_EN_N     1
    gpio_set SCM_CPLD_JTAG_EN_N     1
    gpio_set FCM_CPLD_JTAG_EN_N     1
    gpio_set PWR_CPLD_HITLESS       0
    gpio_set FCM_CPLD_HITLESS       0
    gpio_set SCM_CPLD_HITLESS       0
    gpio_set SMB_CPLD_HITLESS       0
    gpio_set BMC_FCM_SEL            1
    gpio_set BMC_SCM_CPLD_EN        1
}

trap 'rm -rf /tmp/fcmcpld_update' INT TERM QUIT EXIT

echo 1 > /tmp/fcmcpld_update

wedge_prepare_cpld_update

enable_jtag_chain

case $2 in
    hw)
        cpldprog -p "${img}"
        ;;
    sw)
        ispvm -f 1000 dll $DLL_PATH "${img}"
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
