#!/bin/bash
#
# Copyright 2022-present Facebook. All Rights Reserved.
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
# shellcheck disable=SC2235
. /usr/local/bin/openbmc-utils.sh

trap handle_signal INT TERM QUIT

# mdio prints in the format "PAGE_HEX/OFFSET_HEX VALUE_HEX == VALUE_BIN\n"
MDIO_READ_PATTERN="^.* (0x[a-fA-F0-9]+).*$"

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
       ret="0 1 2 IMP"
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

do_read_lnksts() {
    local page offset result ports port val
    page=0x1
    offset=0x0
    result=$(oob-mdio-util.sh read16 "$page" "$offset" \
        | sed -E "s/$MDIO_READ_PATTERN/\1/")
    ports="$*"

    printf "%sLink status:\n" "$tab"
    tab="	$tab"

    for port in $ports; do
        port=${port^^}
        val=$(port_to_val "$port")
        val=$(( (result >> val) & 0x1 ))
        case "$val" in
            0)
                printf "%sPort: %s Link Down\n" "$tab" "$port"
                ;;
            1)
                printf "%sPort: %s Link Up\n" "$tab" "$port"
                ;;
        esac
    done
    tab=${tab:1}
}

do_read_spdsts() {
    local page offset result ports port val
    page=0x1
    offset=0x4
    result=$(oob-mdio-util.sh read32 "$page" "$offset" \
        | sed -E "s/$MDIO_READ_PATTERN/\1/")
    ports="$*"

    printf "%sLink speed:\n" "$tab"
    tab="	$tab"

    for port in $ports; do
        port=${port^^}
        val=$(port_to_val "$port")
        val=$(( (result >> (val * 2)) & 0x3 ))
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
        esac
    done
    tab=${tab:1}
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
    "--help")
        usage
        exit 1
        ;;
    *)
        read_status "$@"
        ;;
esac
