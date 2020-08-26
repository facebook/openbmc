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

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

DLL_PATH=/usr/lib/libcpldupdate_dll_ast_jtag.so

usage() {
    prog=$(basename "$0")
    echo "Usage: $prog -s <CPLD_TYPE> -f <img_file> <hw|sw>"
    echo
    echo "CPLD_TYPE: ( FCM | SCM | SMB | PDB )"
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

if [ $# -lt 5 ]; then
    usage
    exit 1
fi

CPLD_TYPE="$2"
UPDATE_IMG="$4"

enable_fcm_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN_R      1
    gpio_set_value BMC_FCM_SEL_R          0

    gpio_set_value PWR_CPLD_JTAG_EN_N     1
    gpio_set_value FCM_CPLD_JTAG_EN_N     0
    gpio_set_value SYS_CPLD_JTAG_EN_N     1
    gpio_set_value SCM_CPLD_JTAG_EN_N     1
    gpio_set_value BMC_SCM_CPLD_EN_R      1
    gpio_set_value BMC_FPGA_JTAG_EN       1

    gpio_set_value DOM_FPGA1_JTAG_EN_N_R  1
    gpio_set_value DOM_FPGA2_JTAG_EN_N_R  1
    gpio_set_value PWR_CPLD_HITLESS_R     1
}

enable_smb_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN_R      1
    gpio_set_value BMC_FCM_SEL_R          1
    gpio_set_value PWR_CPLD_JTAG_EN_N     1
    gpio_set_value FCM_CPLD_JTAG_EN_N     1

    gpio_set_value SYS_CPLD_JTAG_EN_N     0
    gpio_set_value SCM_CPLD_JTAG_EN_N     1
    gpio_set_value BMC_SCM_CPLD_EN_R      1
    gpio_set_value BMC_FPGA_JTAG_EN       1

    gpio_set_value DOM_FPGA1_JTAG_EN_N_R  1
    gpio_set_value DOM_FPGA2_JTAG_EN_N_R  1
    gpio_set_value PWR_CPLD_HITLESS_R     1
}

enable_scm_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN_R      1
    gpio_set_value BMC_FCM_SEL_R          1
    gpio_set_value PWR_CPLD_JTAG_EN_N     1

    gpio_set_value FCM_CPLD_JTAG_EN_N     1
    gpio_set_value SYS_CPLD_JTAG_EN_N     1
    gpio_set_value SCM_CPLD_JTAG_EN_N     0
    gpio_set_value BMC_SCM_CPLD_EN_R      0
    gpio_set_value BMC_FPGA_JTAG_EN       1

    gpio_set_value DOM_FPGA1_JTAG_EN_N_R  1
    gpio_set_value DOM_FPGA2_JTAG_EN_N_R  1
    gpio_set_value PWR_CPLD_HITLESS_R     1
}

enable_pdb_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN_R      1
    gpio_set_value BMC_FCM_SEL_R          1
    gpio_set_value PWR_CPLD_JTAG_EN_N     0

    gpio_set_value FCM_CPLD_JTAG_EN_N     1
    gpio_set_value SYS_CPLD_JTAG_EN_N     1
    gpio_set_value SCM_CPLD_JTAG_EN_N     1
    gpio_set_value BMC_SCM_CPLD_EN_R      1
    gpio_set_value BMC_FPGA_JTAG_EN       1

    gpio_set_value DOM_FPGA1_JTAG_EN_N_R  1
    gpio_set_value DOM_FPGA2_JTAG_EN_N_R  1
    gpio_set_value PWR_CPLD_HITLESS_R     1
}

disable_jtag_chain(){

    gpio_set_value BMC_JTAG_MUX_IN_R      0
    gpio_set_value BMC_FCM_SEL_R          1

    gpio_set_value FCM_CPLD_JTAG_EN_N     1
    gpio_set_value SYS_CPLD_JTAG_EN_N     1
    gpio_set_value SCM_CPLD_JTAG_EN_N     1
    gpio_set_value BMC_SCM_CPLD_EN_R      1
    gpio_set_value BMC_FPGA_JTAG_EN       1
    gpio_set_value PWR_CPLD_JTAG_EN_N     1

    gpio_set_value DOM_FPGA1_JTAG_EN_N_R  1
    gpio_set_value DOM_FPGA2_JTAG_EN_N_R  1
    gpio_set_value PWR_CPLD_HITLESS_R     1
}

trap 'rm -rf /tmp/cpld_update' INT TERM QUIT EXIT

echo 1 > /tmp/cpld_update

if [ -e "$UPDATE_IMG" ];then
    if [[  $CPLD_TYPE == "FCM" ]];then
        enable_fcm_jtag_chain
    elif [[  $CPLD_TYPE == "SMB" ]];then
        enable_smb_jtag_chain
    elif [[  $CPLD_TYPE == "SCM" ]];then
        enable_scm_jtag_chain
    elif [[  $CPLD_TYPE == "PDB" ]];then
        enable_pdb_jtag_chain
   else
        echo 'argument '"$CPLD_TYPE"' is wrong'
        exit 1
    fi
else
    echo 'argument '"$UPDATE_IMG"' not exist'
    exit 1
fi

case $5 in
    hw)
        echo "Not support yet"
        usage
        exit 1
        cpldprog -p "${UPDATE_IMG}"
        ;;
    sw)
        ispvm -f 1000 dll $DLL_PATH "${UPDATE_IMG}"
        ;;
    *)
        # default: hw mode
        echo "Not support yet"
        usage
        exit 1
        cpldprog -p "${UPDATE_IMG}"
        ;;
esac

result=$?

# ispvm return 1 when the process exits successfully and return 0 when it fails.
if [ "$5" = "sw" ]; then
    expect=1
else
    expect=0
fi

disable_jtag_chain

# 0 is returned upon upgrade success
if [ $result -eq $expect ]; then
    echo "Upgrade successful."
    exit 0
else
    echo "Upgrade failure. Return code from utility : $result"
    exit 1
fi
