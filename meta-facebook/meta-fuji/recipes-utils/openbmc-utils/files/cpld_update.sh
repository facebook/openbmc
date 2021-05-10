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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

prog="$0"
CPLD_TYPE="$2"
UPDATE_IMG="$4"

chip_ver=$(aspeed_g6_chip_ver)

DLL_PATH=/usr/lib/libcpldupdate_dll_gpio.so
DLL_AST_JTAG_PATH=/usr/lib/libcpldupdate_dll_ast_jtag.so

usage() {
    echo "Usage: $prog -s <CPLD_TYPE> -f <img_file> <hw|sw|i2c>"
    echo
    echo "CPLD_TYPE: ( FCM-T | FCM-B | SCM | SMB | PWR-L | PWR-R | PFR)"
    echo
    echo "img_file: Image file for lattice CPLD"
    echo "  VME file for software mode"
    echo "  JED file for hardware mode"
    echo "  JBC file for Intel PFR FPGA update"
    echo "  HEX file for i2c mode"
    echo "options:"
    echo "  hw: Program the CPLD using JTAG hardware mode"
    echo "  sw: Program the CPLD using JTAG software mode"
    echo "  i2c: Program the CPLD using hardware IP core mode"
    echo
    echo
}

if [ $# -lt 5 ]; then
    usage
    exit 1
fi

enable_fct-t_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN        1
    gpio_set_value FCM_1_CPLD_JTAG_EN_N   0
    gpio_set_value BMC_FCM_1_SEL          0
    gpio_set_value FCM_2_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_2_SEL          1
    gpio_set_value SYS_CPLD_JTAG_EN_N     1
    gpio_set_value BMC_SCM_CPLD_JTAG_EN_N 1
    gpio_set_value BMC_FPGA_JTAG_EN       1
}

enable_pfr_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN        1
    gpio_set_value FCM_1_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_1_SEL          1
    gpio_set_value FCM_2_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_2_SEL          1
    gpio_set_value SYS_CPLD_JTAG_EN_N     1
    gpio_set_value BMC_SCM_CPLD_JTAG_EN_N 1
    gpio_set_value BMC_FPGA_JTAG_EN       0
}

enable_fct-b_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN        1
    gpio_set_value FCM_1_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_1_SEL          1
    gpio_set_value FCM_2_CPLD_JTAG_EN_N   0
    gpio_set_value BMC_FCM_2_SEL          0
    gpio_set_value SYS_CPLD_JTAG_EN_N     1
    gpio_set_value BMC_SCM_CPLD_JTAG_EN_N 1
    gpio_set_value BMC_FPGA_JTAG_EN       1
}

enable_smb_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN        1
    gpio_set_value FCM_1_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_1_SEL          1
    gpio_set_value FCM_2_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_2_SEL          1
    gpio_set_value SYS_CPLD_JTAG_EN_N     0
    gpio_set_value BMC_SCM_CPLD_JTAG_EN_N 1
    gpio_set_value BMC_FPGA_JTAG_EN       1
}

enable_scm_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN        1
    gpio_set_value FCM_1_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_1_SEL          1
    gpio_set_value FCM_2_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_2_SEL          1
    gpio_set_value SYS_CPLD_JTAG_EN_N     1
    gpio_set_value BMC_SCM_CPLD_JTAG_EN_N 0
    gpio_set_value BMC_FPGA_JTAG_EN       1
}

enable_pdb-l_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN        1
    gpio_set_value FCM_1_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_1_SEL          1
    gpio_set_value FCM_2_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_2_SEL          1
    gpio_set_value SYS_CPLD_JTAG_EN_N     1
    gpio_set_value BMC_SCM_CPLD_JTAG_EN_N 1
    gpio_set_value BMC_FPGA_JTAG_EN       1
    gpio_set_value PDB_L_HITLESS    0
}

enable_pdb-r_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN        1
    gpio_set_value FCM_1_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_1_SEL          1
    gpio_set_value FCM_2_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_2_SEL          1
    gpio_set_value SYS_CPLD_JTAG_EN_N     1
    gpio_set_value BMC_SCM_CPLD_JTAG_EN_N 1
    gpio_set_value BMC_FPGA_JTAG_EN       1
    gpio_set_value PDB_R_HITLESS    0
}

disable_jtag_chain(){
    gpio_set_value BMC_JTAG_MUX_IN        0
    gpio_set_value FCM_1_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_1_SEL          1
    gpio_set_value FCM_2_CPLD_JTAG_EN_N   1
    gpio_set_value BMC_FCM_2_SEL          1
    gpio_set_value SYS_CPLD_JTAG_EN_N     1
    gpio_set_value BMC_SCM_CPLD_JTAG_EN_N 1
    gpio_set_value BMC_FPGA_JTAG_EN       1
    gpio_set_value PDB_L_HITLESS    1
    gpio_set_value PDB_R_HITLESS    1
}


trap 'rm -rf /tmp/fcmcpld_update' INT TERM QUIT EXIT

echo 1 > /tmp/fcmcpld_update

if [ -e "$UPDATE_IMG" ];then
    if [[  $CPLD_TYPE == "FCM-T" ]];then
        enable_fct-t_jtag_chain
    elif [[  $CPLD_TYPE == "FCM-B" ]];then
        enable_fct-b_jtag_chain
    elif [[  $CPLD_TYPE == "PFR" ]];then
        enable_pfr_jtag_chain
    elif [[  $CPLD_TYPE == "SMB" ]];then
        enable_smb_jtag_chain
    elif [[  $CPLD_TYPE == "SCM" ]];then
        enable_scm_jtag_chain
    elif [[  $CPLD_TYPE == "PWR-L" ]];then
        enable_pdb-l_jtag_chain
    elif [[  $CPLD_TYPE == "PWR-R" ]];then
        enable_pdb-r_jtag_chain
    else
        echo 'argument '"$CPLD_TYPE"' is wrong'
        exit 1
    fi
else
    echo 'argument '"$UPDATE_IMG"' not exist'
    exit 1
fi

expect=0

cpld_update_sw_mode(){
        for n in {1..5}
        do
                echo "Program $CPLD_TYPE $n times"
                if [[  $CPLD_TYPE == "PWR-L" ]];then
                # ispvm success return code is 1
                expect=1
                ispvm -f 100 dll $DLL_PATH "${UPDATE_IMG}" --tdo PDB_L_JTAG_TDO --tdi PDB_L_JTAG_TDI --tms PDB_L_JTAG_TMS --tck PDB_L_JTAG_TCK
                elif [[  $CPLD_TYPE == "PWR-R" ]];then
                # ispvm success return code is 1
                expect=1
                ispvm -f 100 dll $DLL_PATH "${UPDATE_IMG}" --tdo PDB_R_JTAG_TDO --tdi PDB_R_JTAG_TDI --tms PDB_R_JTAG_TMS --tck PDB_R_JTAG_TCK
                elif [[  $CPLD_TYPE == "PFR" ]];then
                jbi -aPROGRAM -ddo_real_time_isp=1 -W "${UPDATE_IMG}"
                else
                # ispvm success return code is 1
                expect=1
                ispvm -f 100 dll $DLL_AST_JTAG_PATH "${UPDATE_IMG}"
                fi

                result=$?
                if [ $result -eq $expect ]; then
                        break
                fi
        done
}

cpld_update_i2c_mode(){
        for n in {1..5}
        do
                echo "Program $CPLD_TYPE $n times"
                if [[  $CPLD_TYPE == "PWR-L" ]];then
                        cpldupdate-i2c 53 0x40 "${UPDATE_IMG}"
                elif [[  $CPLD_TYPE == "PWR-R" ]];then
                        cpldupdate-i2c 61 0x40 "${UPDATE_IMG}"
                else
                echo "Only Power CPLD support I2C Mode !!!"
                fi
                result=$?
                if [ $result -eq $expect ]; then
                        break
                fi
        done
}

cpld_update_hw_mode(){
        if [ "$chip_ver" == "BOARD_FUJI_EVT1" ];then
                cpldprog -p "${UPDATE_IMG}"
        else
                echo "Only AST262x A3 chip can support HW Mode !!!"
                exit 1
        fi
        result=$?
}

case $5 in
    hw)
        cpld_update_hw_mode
        ;;
    sw)
        cpld_update_sw_mode
        ;;
    i2c)
        cpld_update_i2c_mode
        ;;
    *)
        # default: hw mode
        cpld_update_hw_mode
        ;;
esac

disable_jtag_chain

# 0 is returned upon upgrade success
if [ $result -eq $expect ]; then
    echo "Upgrade successful."
    exit 0
else
    echo "Upgrade failure. Return code from utility : $result"
    exit 1
fi
