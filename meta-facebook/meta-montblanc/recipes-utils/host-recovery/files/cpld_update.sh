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
    n=1
    while [ "${n}" -le 5 ]; do
        echo "Program $CPLD_TYPE $n times"

        expect=0
        jbi -aPROGRAM -ddo_real_time_isp=1 -W "${UPDATE_IMG}"

        result=$?
        if [ $result -eq $expect ]; then
                break
        fi
        n=$((n + 1))
    done
}

trap 'rm -rf /tmp/cpld_update && disable_jtag_chain' INT TERM QUIT EXIT

echo 1 > /tmp/cpld_update

expect=0

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

disable_jtag_chain

# 0 is returned upon upgrade success
if [ $((result)) -eq $((expect)) ]; then
    echo "Upgrade successful."
    exit 0
fi
