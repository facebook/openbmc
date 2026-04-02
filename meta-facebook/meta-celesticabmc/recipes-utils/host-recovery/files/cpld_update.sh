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
# shellcheck source=/dev/null
. /usr/local/bin/openbmc-utils.sh

prog="$0"
CPLD_TYPE="$1"
UPDATE_IMG="$2"
MODE="$3"

DLL_PATH=/usr/lib/libcpldupdate_dll_ast_jtag.so

export_gpio()
{
    setup_gpio MUX_JTAG_SEL_1 GPIOF5 out 0 > /dev/null 2>&1
    setup_gpio MUX_JTAG_SEL_0 GPIOF0 out 0 > /dev/null 2>&1
    sleep 1
}

unexport_gpio()
{
    gpiocli --shadow MUX_JTAG_SEL_1 unexport > /dev/null 2>&1
    gpiocli --shadow MUX_JTAG_SEL_0 unexport > /dev/null 2>&1
}


usage() {
    echo "Usage: $prog <CPLD_TYPE> <img_file> <hw|sw>"
    echo
    echo "CPLD_TYPE: ( COME, SCM )"
    echo "  COME: COMe CPLD"
    echo "  SCM: Internal flash SCM CPLD (default SCMCPLD boot from external flash)"
    echo
    echo "img_file: Image file for lattice CPLD"
    echo "  VME file for software mode"
    echo "  JED file for hardware mode"
    echo "  JBC file for Intel PFR FPGA update software mode"
    echo "  HEX file for i2c mode"
    echo "options:"
    echo "  hw: Program the CPLD using JTAG hardware mode"
    echo "  sw: Program the CPLD using JTAG software mode"
    echo
    echo
}

if [ $# -lt 2 ]; then
    usage
    exit 1
fi

#
#  Multiplexer Diagram
#                        buffer
#                       ┌──────┐ MCB_CPLD_JTAG_EN_L
# [ HEADER ] ────────┬─ | U157 | ─────────────────────────────── [ MCB CPLD ]
#                    │  └──────┘ 
#                    │  ┌──────┐ SMB_CPLD_JTAG_EN_L
#                    ├─ │      │ ─────────────────────────────── [ SMB CPLD ]
#                    │  │ U127 │ SCM_CPLD_JTAG_EN_L     U27
#                    └─ │      │ ─────────────────── │ 0    │
#               U28     └──────┘                     │    C │ ── [ SCM CPLD ]
#             │    0 │ ───────────────────────────── │ 1    │
#   [ BMC ] ─ │ C    │                            MUX_JTAG_SEL_0
#             │    1 │ ───────────────────────────────────────── [ COMe CPLD ]
#          MUX_JTAG_SEL_1
#
# Note: support only COMe CPLD and SCM CPLD
#

enable_come_jtag_chain(){
    gpiocli -s MUX_JTAG_SEL_0 set-value 0
    gpiocli -s MUX_JTAG_SEL_1 set-value 1
}

enable_scm_jtag_chain(){
    gpiocli -s MUX_JTAG_SEL_0 set-value 1
    gpiocli -s MUX_JTAG_SEL_1 set-value 0
}

disable_jtag_chain(){
    gpiocli -s MUX_JTAG_SEL_0 set-value 0
    gpiocli -s MUX_JTAG_SEL_1 set-value 0
}

cpld_update_sw_mode(){
    n=1
    netlake_identify
    netlake_type=$?
    while [ "${n}" -le 5 ]; do
        echo "Program $CPLD_TYPE $n times"

        if [ "$CPLD_TYPE" = "SCM" ]; then
            expect=1
            ispvm -f 1000 dll $DLL_PATH "${UPDATE_IMG}"
        elif [ "$CPLD_TYPE" = "COME" ]; then            
            if [ "$netlake_type" -eq 0 ]; then
                echo 'This is Netlake 1.0, use jbi for COMe CPLD update'
                expect=0
                jbi -aPROGRAM -ddo_real_time_isp=1 -W "${UPDATE_IMG}"
            # Netlake2.0 FPGA is using vme format
            elif [ "$netlake_type" -eq 1 ]; then
                echo 'This is Netlake 2.0, use ispvm for COMe CPLD update'
                expect=1
                ispvm -f 1000 dll $DLL_PATH "${UPDATE_IMG}"                
            else
                echo 'failed to identify COMe type: Netlake 1.0 or 2.0'
            fi
        else
            echo "CPLD_TYPE is wrong"
            exit 1
        fi

        result=$?
        if [ $result -eq $expect ]; then
                break
        fi
        n=$((n + 1))
    done
}

trap 'rm -rf /tmp/cpld_update && disable_jtag_chain && unexport_gpio' INT TERM QUIT EXIT
echo 1 > /tmp/cpld_update
expect=0

export_gpio
case "$CPLD_TYPE" in
    COME)
        enable_come_jtag_chain
        ;;
    SCM)
        enable_scm_jtag_chain
        ;;
    *)
        echo 'argument '"$CPLD_TYPE"' is wrong'
        usage
        exit 1
        ;;
esac

case "$MODE" in
    hw)
        echo 'HW mode not support'
        ;;
    sw)
        cpld_update_sw_mode
        ;;
    *)
        # default: sw mode
        cpld_update_sw_mode
        ;;
esac

# 0 is returned upon upgrade success
if [ $((result)) -eq $((expect)) ]; then
    echo "Upgrade successful."
    exit 0
fi
