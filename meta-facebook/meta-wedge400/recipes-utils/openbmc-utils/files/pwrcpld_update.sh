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

if [ $# -ne 1 ]; then
    exit -1
fi

img="$1"

source /usr/local/bin/openbmc-utils.sh

KERNEL_VERSION=`uname -r`
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
    DLL_PATH=/usr/lib/libcpldupdate_dll_jtag.so
else
    DLL_PATH=/usr/lib/libcpldupdate_dll_ast_jtag.so
fi

enable_jtag_chain(){
    gpio_set BMC_JTAG_MUX_IN        1
    gpio_set PWR_CPLD_JTAG_EN_N     0
    gpio_set SYS_CPLD_JTAG_EN_N     1
    gpio_set SCM_CPLD_JTAG_EN_N     1
    gpio_set FCM_CPLD_JTAG_EN_N     1
    gpio_set PWR_CPLD_HITLESS       1
    gpio_set FCM_CPLD_HITLESS       0
    gpio_set SCM_CPLD_HITLESS       0
    gpio_set SMB_CPLD_HITLESS       0
    gpio_set BMC_FCM_SEL            1
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

trap 'rm -rf /tmp/pwrcpld_update' INT TERM QUIT EXIT

echo 1 > /tmp/pwrcpld_update

enable_jtag_chain
ispvm -f 1000 dll $DLL_PATH "${img}"
result=$?
disable_jtag_chain

# 1 is returned upon upgrade success
if [ $result -eq 1 ]; then
    echo "Upgrade successful."
    exit 0
else
    echo "Upgrade failure. Return code from utility : $result"
    exit 1
fi

