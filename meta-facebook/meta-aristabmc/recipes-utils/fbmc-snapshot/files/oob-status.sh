#!/bin/bash
#
# Copyright 2026-present Facebook. All Rights Reserved.
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
# Report BCM53134 management-switch port status on Arista AST2700 BMCs.
#
# shellcheck disable=SC1091
# shellcheck disable=SC2068
# shellcheck disable=SC2235
set -o pipefail

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

trap handle_signal INT TERM QUIT

# mdio prints in the format "PAGE_HEX/OFFSET_HEX VALUE_HEX == VALUE_BIN\\n"
MDIO_READ_PATTERN="^.* (0x[a-fA-F0-9]+).*$"
OOB_MDIO_UTIL=/usr/local/bin/oob-mdio-util.sh
MII_STATUS_WORD=1
MII_STATUS_LINK_ST=0x4

# Global variable to stack tabs for formatting
tab=""

usage() {
    echo "Read BCM53134P (oob switch) port status registers"
    echo ""
    echo "# To dump all port status"
    echo "oob-status.sh"
    echo "# To dump port status for ports in 0, 1, 2, 3, IMP"
    echo "oob-status.sh [Port(s)]"
    echo "# To print only link status"
    echo "oob-status.sh link_status [Port(s)]"
    echo "# To print only link speed"
    echo "oob-status.sh link_speed [Port(s)]"
    echo "# To print counters"
    echo "oob-status.sh counters [--detail] [Port(s)]"
    echo ""
    echo "Example:"
    echo ""
    echo "oob-status.sh link_status 0 IMP"
    echo "Link Status:"
    echo "	Port 0: Link Up"
    echo "	Port IMP: Link Up"
}

handle_signal() {
    echo "Exiting because of signal" >&2
    exit 1
}

check_sts_args() {
    local port
    ret="$*"
    if [ $# -lt 1 ]; then
        ret="0 1 2 3 IMP"
    else
        for port in "$@"; do
            if [ "${port^^}" != "IMP" ] && (! [[ "$port" =~ ^[0-9]+$ ]] \
                || [ "$port" -lt 0 ] || [ "$port" -gt 3 ]); then
                echo "Invalid port $port"
                exit 1
            fi
        done
    fi
}

port_to_val() {
    local val
    if [ "$1" == "IMP" ]; then
        val=8
    else
        val="$1"
    fi
    echo "$val"
}

mdio_enabled=1
check_mdio_util_result() {
    local rc=$1
    if [ "$rc" -eq 2 ]; then
        mdio_enabled=0
    fi
}

get_link_status() {
    local result ports port val
    ports="$*"
    if [ $mdio_enabled -eq 1 ]; then
        result=$("$OOB_MDIO_UTIL" read16 "$page" "$offset" \
            | sed -E "s/$MDIO_READ_PATTERN/\1/")
        check_mdio_util_result $?
        if [ $mdio_enabled -eq 1 ]; then
            echo "$result"
            return
        fi
    fi

    result=0x100
    for port in $ports; do
        port=${port^^}
        if [ "$port" == "IMP" ]; then
            continue
        fi
        val=$("$OOB_MDIO_UTIL" mii-read "$port" "$MII_STATUS_WORD")
        if [ $((val & MII_STATUS_LINK_ST)) -eq 0 ]; then
            val=$("$OOB_MDIO_UTIL" mii-read "$port" "$MII_STATUS_WORD")
        fi
        if [ $((val & MII_STATUS_LINK_ST)) -ne 0 ]; then
            result=$((result | (1 << port)))
        fi
    done

    echo "$result"
}

do_read_lnksts() {
    local page offset result ports port val
    page=0x1
    offset=0x0
    ports="$*"
    result=$(get_link_status "$ports")

    printf "%sLink status:\n" "$tab"
    tab="	$tab"
    for port in $ports; do
        port=${port^^}
        val=$(port_to_val "$port")
        if [ -n "$result" ]; then
            val=$(((result >> val) & 0x1))
        else
            val="unknown"
        fi
        case "$val" in
            0)
                printf "%sPort: %s Link Down\n" "$tab" "$port"
                ;;
            1)
                printf "%sPort: %s Link Up\n" "$tab" "$port"
                ;;
            *)
                printf "%sPort: %s Link %s\n" "$tab" "$port" "$val"
                ;;
        esac
    done
    tab=${tab:1}
}

do_read_spdsts() {
    local page offset result ports port val
    page=0x1
    offset=0x4
    if [ $mdio_enabled -eq 1 ]; then
        result=$("$OOB_MDIO_UTIL" read32 "$page" "$offset" \
            | sed -E "s/$MDIO_READ_PATTERN/\1/")
        check_mdio_util_result $?
    fi
    if [ $mdio_enabled -eq 0 ]; then
        result=""
    fi
    ports="$*"

    printf "%sLink speed:\n" "$tab"
    tab="	$tab"
    for port in $ports; do
        port=${port^^}
        val=$(port_to_val "$port")
        if [ -n "$result" ]; then
            val=$(((result >> (val * 2)) & 0x3))
        else
            val="unknown"
        fi
        case "$val" in
            0)
                printf "%sPort: %s 10 Mb/s\n" "$tab" "$port"
                ;;
            1)
                printf "%sPort: %s 100 Mb/s\n" "$tab" "$port"
                ;;
            2)
                printf "%sPort: %s 1000 Mb/s\n" "$tab" "$port"
                ;;
            *)
                printf "%sPort: %s %s\n" "$tab" "$port" "$val"
                ;;
        esac
    done
    tab=${tab:1}
}

PORT0_3_PAGE_BASE=0x20
PORT5_PAGE_BASE=0x25
PORTIMP_PAGE_BASE=0x28

port_to_counter_page_base() {
    local val
    if [ "$1" -eq 5 ]; then
        val="$PORT5_PAGE_BASE"
    elif [ "$1" -eq 8 ]; then
        val="$PORTIMP_PAGE_BASE"
    else
        val=$((PORT0_3_PAGE_BASE + $1))
    fi
    echo "$val"
}

COUNTER_REGISTERS=(
    "txOctets 0x0 64 false"
    "txDropPkts 0x8 32 false"
    "txQpktQ0 0xc 32 true"
    "txBroadcastPkts 0x10 32 false"
    "txMulticastPkts 0x14 32 false"
    "txUnicastPkts 0x18 32 false"
    "txCollisions 0x1c 32 false"
    "txSingleCollisions 0x20 32 true"
    "txMultipleCollisions 0x24 32 true"
    "txDeferredTransmits 0x28 32 true"
    "txLateCollisions 0x2c 32 true"
    "txExcessiveCollisions 0x30 32 true"
    "txFramesInDiscard 0x34 32 true"
    "txPausePackets 0x38 32 true"
    "txQpktQ1 0x3c 32 true"
    "txQpktQ2 0x40 32 true"
    "txQpktQ3 0x44 32 true"
    "txQpktQ4 0x48 32 true"
    "txQpktQ5 0x4c 32 true"
    "rxOctets 0x50 64 false"
    "rxUndersizePkts 0x58 32 true"
    "rxPausePkts 0x5c 32 true"
    "rxPkts64Octets 0x60 32 true"
    "rxPkts65to127Octets 0x64 32 true"
    "rxPkts128to255Octets 0x68 32 true"
    "rxPkts256to511Octets 0x6c 32 true"
    "rxPkts512to1023Octets 0x70 32 true"
    "rxPkts1024toMaxPktOctets 0x74 32 true"
    "rxOversizePkts 0x78 32 true"
    "rxJabbers 0x7c 32 false"
    "rxAlignmentErrors 0x80 32 false"
    "rxFCSErrors 0x84 32 false"
    "rxGoodOctets 0x88 64 false"
    "rxDropPkts 0x90 32 false"
    "rxUnicastPkts 0x94 32 false"
    "rxMulticastPkts 0x98 32 false"
    "rxBroadcastPkts 0x9c 32 false"
    "rxSAChanges 0xa0 32 false"
    "rxFragments 0xa4 32 false"
    "rxJumboPkts 0xa8 32 true"
    "rxSymblErrs 0xac 32 false"
    "rxInRangeErrCount 0xb0 32 false"
    "rxOutRangeErrCount 0xb4 32 false"
    "eeeLpiEvents 0xb8 32 false"
    "eeeLpiDuration 0xbc 32 false"
    "rxDiscards 0xc0 32 false"
    "txQpktQ6 0xc8 32 false"
    "txQpktQ7 0xcc 32 false"
    "txPkts64Octets 0xd0 32 true"
    "txPkts65to127Octets 0xd4 32 true"
    "txPkts128to255Octets 0xd8 32 true"
    "txPkts256to511Octets 0xdc 32 true"
    "txPkts512to1023Octets 0xe0 32 true"
    "txPkts1024toMaxPktOctets 0xe4 32 true"
)

do_read_counter() {
    local page=$1
    local name=$2
    local addr=$3
    local data_size=$4
    local num_bits result

    # COUNTER_REGISTERS records register widths in bits, matching the
    # oob-mdio-util read8/read16/read32/read64 command suffixes.
    num_bits=$data_size
    result=$("$OOB_MDIO_UTIL" "read$num_bits" "$page" "$addr" \
        | sed -E "s/$MDIO_READ_PATTERN/\1/")
    check_mdio_util_result $?
    if [ $mdio_enabled -eq 1 ]; then
        echo "$tab$name: $result"
    fi
}

do_read_counters() {
    local detail=$1
    shift
    local ports="$*"
    local port_str port page reg_def_entry is_detail

    for port_str in $ports; do
        port=$(port_to_val "${port_str^^}")
        printf "%sPort %d\n" "$tab" "$port"
        tab="	$tab"
        page=$(port_to_counter_page_base "$port")
        for reg_def_entry in "${COUNTER_REGISTERS[@]}"; do
            read -ra reg_def <<< "$reg_def_entry"
            is_detail="${reg_def[3]}"
            if [ "$is_detail" = "true" ] && [ "$detail" != "True" ]; then
                continue
            fi
            do_read_counter "$page" ${reg_def[@]}
            if [ $mdio_enabled -eq 0 ]; then
                echo "MDIO bus transactions not enabled. Counters not available"
                return
            fi
        done
        tab=${tab:1}
    done
}

read_lnksts() {
    check_sts_args "$@"
    do_read_lnksts "$ret"
}

read_spdsts() {
    check_sts_args "$@"
    do_read_spdsts "$ret"
}

read_status() {
    check_sts_args "$@"
    printf "%sStatus:\n" "$tab"
    tab="	$tab"
    do_read_lnksts "$ret"
    do_read_spdsts "$ret"
    tab=${tab:1}
}

read_counters() {
    detail="False"
    if [ "$#" -gt 0 ] && [ "$1" = "--detail" ]; then
        detail="True"
        shift
    fi
    check_sts_args "$@"
    do_read_counters "$detail" "$ret"
}

# Only allow one instance of script to run at a time.
script=$(realpath "$0")
exec 100< "$script"
flock -n 100 || { echo "ERROR: $0 already running" && exit 1; }

command="$1"

case "$command" in
    link_status)
        shift
        read_lnksts "$@"
        ;;
    link_speed)
        shift
        read_spdsts "$@"
        ;;
    counters)
        shift
        read_counters "$@"
        ;;
    "--help")
        usage
        exit 1
        ;;
    *)
        read_status "$@"
        ;;
esac
