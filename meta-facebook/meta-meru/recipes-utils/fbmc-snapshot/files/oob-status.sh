#!/bin/bash
#
# Copyright 2023-present Facebook. All Rights Reserved.
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

# shellcheck disable=SC1091
# shellcheck disable=SC2068
# shellcheck disable=SC2235
set -o pipefail

. /usr/local/bin/openbmc-utils.sh

trap handle_signal INT TERM QUIT

# MERUTODO: special-case P2 as this may not be supported

# mdio prints in the format "PAGE_HEX/OFFSET_HEX VALUE_HEX == VALUE_BIN\n"
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
    echo "# To dump port status for ports in 0, 1, 2, IMP"
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
    # Convert arguments to port numbers
    local port val
    ret="$*"
    if [ $# -lt 1 ]; then
       # No arguments: do all ports
       ret="0 1 2 3 IMP"
    else
        for port in "$@"; do
            if [ "${port^^}" != "IMP" ] && (! [[ "$port" =~ ^[0-9]+$ ]] \
                || [ "$port" -lt 0 ] || [ "$port" -gt 2 ]); then
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
    local result
    local ports
    local val
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

    # Retrieve the link statuses using the mii status word for
    # each port. The IMP port will always be up.
    result=0x100
    for port in $ports; do
        port=${port^^}
        if [ "$1" == "IMP" ]; then
            continue
        fi
        val=$("$OOB_MDIO_UTIL" mii-read "$port" "$MII_STATUS_WORD")
        if [ $((val & MII_STATUS_LINK_ST)) -eq 0 ]; then
            # The link status bit is latched when the link fails and
            # is unlatched on the next read. Therefore, the status must
            # be read a second time to get the current state.
            val=$("$OOB_MDIO_UTIL" mii-read "$port" "$MII_STATUS_WORD")
        fi
        if [ $((val & MII_STATUS_LINK_ST)) -ne 0 ]; then
            result=$((result | (1 << port) ))
        fi
    done

    echo $result
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
            val=$(( (result >> val) & 0x1 ))
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
            val=$(( (result >> (val * 2)) & 0x3 ))
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
        val=$(("$PORT0_3_PAGE_BASE" + "$1"))
    fi
    echo "$val"
}

COUNTER_REGISTERS=(
    "txOctets 0x0 8 False"
    "txDropPkts 0x8 4 False"
    "txQpktQ0 0xc 4 True"
    "txBroadcastPkts 0x10 4 False"
    "txMulticastPkts 0x14 4 False"
    "txUnicastPkts 0x18 4 False"
    "txCollisions 0x1c 4 False"
    "txSingleCollisions 0x20 4 True"
    "txMultipleCollisions 0x24 4 True"
    "txDeferredTransmits 0x28 4 True"
    "txLateCollisions 0x2c 4 True"
    "txExcessiveCollisions 0x30 4 True"
    "txFramesInDiscard 0x34 4 True"
    "txPausePackets 0x38 4 True"
    "txQpktQ1 0x3c 4 True"
    "txQpktQ2 0x40 4 True"
    "txQpktQ3 0x44 4 True"
    "txQpktQ4 0x48 4 True"
    "txQpktQ5 0x4c 4 True"
    "rxOctets 0x50 8 False"
    "rxUndersizePkts 0x58 4 True"
    "rxPausePkts 0x5c 4 True"
    "rxPkts64Octets 0x60 4 True"
    "rxPkts65to127Octets 0x64 4 True"
    "rxPkts128to255Octets 0x68 4 True"
    "rxPkts256to511Octets 0x6c 4 True"
    "rxPkts512to1023Octets 0x70 4 True"
    "rxPkts1024toMaxPktOctets 0x74 4 True"
    "rxOversizePkts 0x78 4 True"
    "rxJabbers 0x7c 4 False"
    "rxAlignmentErrors 0x80 4 False"
    "rxFCSErrors 0x84 4 False"
    "rxGoodOctets 0x88 8 False"
    "rxDropPkts 0x90 4 False"
    "rxUnicastPkts 0x94 4 False"
    "rxMulticastPkts 0x98 4 False"
    "rxBroadcastPkts 0x9c 4 False"
    "rxSAChanges 0xa0 4 False"
    "rxFragments 0xa4 4 False"
    "rxJumboPkts 0xa8 4 True"
    "rxSymblErrs 0xac 4 False"
    "rxInRangeErrCount 0xb0 4 False"
    "rxOutRangeErrCount 0xb4 4 False"
    "eeeLpiEvents 0xb8 4 False"
    "eeeLpiDuration 0xbc 4 False"
    "rxDiscards 0xc0 4 False"
    "txQpktQ6 0xc8 4 True"
    "txQpktQ7 0xcc 4 True"
    "txPkts64Octets 0xd0 4 True"
    "txPkts65to127Octets 0xd4 4 True"
    "txPkts128to255Octets 0xd8 4 True"
    "txPkts256to511Octets 0xdc 4 True"
    "txPkts512to1023Octets 0xe0 4 True"
    "txPkts1024toMaxPktOctets 0xe4 4 True"
)

do_read_counter() {
    local page name addr data_size
    page="$1"
    name="$2"
    addr="$3"
    data_size="$4"
    num_bits=$(("$data_size" * 8))

    result=$("$OOB_MDIO_UTIL" "read$num_bits" "$page" "$addr" \
        | sed -E "s/$MDIO_READ_PATTERN/\1/")
    check_mdio_util_result $?
    if [ $mdio_enabled -eq 1 ]; then
        echo "$tab$name: $result"
    fi
}

do_read_counters() {
    local detail
    detail="$1"
    shift
    ports="$*"

    for port_str in $ports; do
        port=$(port_to_val "${port_str^^}")
        printf "%sPort %d\n" "$tab" "$port"
        tab="	$tab"
        page=$(port_to_counter_page_base "$port")
        for reg_def_entry in "${COUNTER_REGISTERS[@]}"; do
            read -ra reg_def <<< "$reg_def_entry"
            is_detail="${reg_def[3]}"
            if [ "$is_detail" = "True" ] && [ "$detail" == "False" ]; then
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
