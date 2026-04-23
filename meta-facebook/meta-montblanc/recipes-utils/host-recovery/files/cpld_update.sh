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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
# shellcheck source=/dev/null
. /usr/local/bin/openbmc-utils.sh

prog="$0"
CPLD_TYPE="$2"
UPDATE_IMG="$4"
MODE="$5"

DLL_PATH=/usr/lib/libcpldupdate_dll_ast_jtag.so

usage() {
    echo "Usage: $prog -s <CPLD_TYPE> -f <img_file> <hw|sw>"
    echo
    echo "CPLD_TYPE: ( COME )"
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

if [ $# -lt 5 ]; then
    usage
    exit 1
fi

#
#  Multiplexer Diagram
#
#                         SEL_0
#       from HEADER  >>> |0\  |
#                        |  >C| >> SCM CPLD
#              |  /0| >> |1/  |
#  BMC JTAG >> |C<  |
#              |  \1| >>>  COMe CPLD 
#               SEL_1
#

enable_scm_jtag_chain(){
    gpiocli -s MUX_JTAG_SEL_0 set-value 1
    gpiocli -s MUX_JTAG_SEL_1 set-value 0
}

enable_come_jtag_chain(){
    gpiocli -s MUX_JTAG_SEL_0 set-value 0
    gpiocli -s MUX_JTAG_SEL_1 set-value 1
}

disable_jtag_chain(){
    gpiocli -s MUX_JTAG_SEL_0 set-value 0
    gpiocli -s MUX_JTAG_SEL_1 set-value 0
}

cpld_update_sw_mode(){
    update_cmd=''
    expect=''

    netlake_identify
    ret="$?"

    if [ "$ret" -eq 1 ]; then
        # NL2 system (bit 7 is 1), use ispvm
        if [ ! "${UPDATE_IMG##*.}" = "vme" ]; then
            echo "Error: NL2 system requires a .vme file, but got ${UPDATE_IMG##*/}"
            return 1
        fi
        echo "This is Netlake 2.0, using ispvm for .vme file."
        update_cmd="ispvm -f 1000 dll \"$DLL_PATH\" \"${UPDATE_IMG}\""
        expect=1
    elif [ "$ret" -eq 0 ]; then
        # NL1 system (bit 7 is 0), use jbi
        if [ ! "${UPDATE_IMG##*.}" = "jbc" ]; then
            echo "Error: NL1 system requires a .jbc file, but got ${UPDATE_IMG##*/}"
            return 1
        fi
        echo "This is Netlake 1.0, using jbi for .jbc file."
        update_cmd="jbi -aPROGRAM -ddo_real_time_isp=1 -W \"${UPDATE_IMG}\""
        expect=0
    else
        # Cannot determine system type. Abort.
        echo "Error: Failed to read COMe CPLD to identify COMe type. Aborting update."
        return 1
    fi

    n=1
    while [ "${n}" -le 5 ]; do
        echo "Program $CPLD_TYPE $n times"

        eval "$update_cmd"

        if [ "$?" -eq "$expect" ]; then
            return 0
        fi
        n=$((n + 1))
    done

    echo "Update command failed after 5 retries."
    return 1
}

trap 'rm -rf /tmp/cpld_update && disable_jtag_chain' INT TERM QUIT EXIT

echo 1 > /tmp/cpld_update

if [ ! -e "$UPDATE_IMG" ];then
    echo 'argument '"$UPDATE_IMG"' not exist'
    exit 1
fi

if [ "$CPLD_TYPE" = "SCM" ];then
    #enable_scm_jtag_chain
    echo "SCM CPLD didn't support yet"
    exit 1
elif [ "$CPLD_TYPE" = "COME" ];then
    enable_come_jtag_chain
else
    echo 'argument '"$CPLD_TYPE"' is wrong'
    exit 1
fi

update_status=1
case "$MODE" in
    hw)
        echo 'HW mode not support'
        ;;
    sw)
        cpld_update_sw_mode
        update_status=$?
        ;;
    *)
        # default: sw mode
        cpld_update_sw_mode
        update_status=$?
        ;;
esac

disable_jtag_chain

if [ "$update_status" -eq 0 ]; then
    echo "Upgrade successful."
    exit 0
fi
