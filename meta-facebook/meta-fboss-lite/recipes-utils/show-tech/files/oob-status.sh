#!/bin/bash
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

# shellcheck disable=SC1091
# shellcheck disable=SC2235
. /usr/local/bin/openbmc-utils.sh

trap handle_signal INT TERM QUIT

# mdio prints in the format 
# "Write to PHY 0x12.0x0, value is 0x9a00"
# "Read from PHY 0x12.0x1, value is 0x4e09"
MDIO_READ_PATTERN_DETECT="^Read .* value is 0x[a-fA-F0-9]*$"
MDIO_READ_PATTERN_EXTRACT="^Read .* value is (0x[a-fA-F0-9]*)$"

# Global variable to stack tabs for formatting
tab=""

usage() {
    echo "Read 88E6321 (oob switch) port status registers"
    echo ""
    echo "# To dump all port status"
    echo "oob-status.sh"
    echo "# To dump port status for ports in 0, 1, 2, 3, 4, 5, 6"
    echo "oob-status.sh [Port(s)]"
    echo "# To print only link status"
    echo "oob-status.sh link_status [Port(s)]"
    echo "# To print only link speed"
    echo "oob-status.sh link_speed [Port(s)]"
    echo ""
    echo "Example:"
    echo ""
    echo "oob-status.sh link_status 0 3"
    echo "Link Status:"
    echo "	Port 0: Link Up"
    echo "	Port 3: Link Up"

}

handle_signal() {
    echo "Exiting because of signal" >&2
    exit 1
}

check_sts_args() {
    # Convert arguments to port numbers
    local port
    ret="$*"
    if [ $# -lt 1 ]; then
       # No arguments: do all ports
       ret="0 1 2 3 4 5 6"
    else
        for port in "$@"; do
            if [ "$port" -lt 0 ] || [ "$port" -gt 6 ]; then
                echo "Invalid port $port"
                exit 1
            fi
        done
    fi
    echo "$ret"
}

do_read_lnksts() {
    local result ports port val
    ports="$*"

    printf "%sLink status:\n" "$tab"
    tab="	$tab"

    for port in $ports; do
        port=${port^^}
        result=$(oob-mdio-util.sh read "$port" 0x0 \
            | grep -e "$MDIO_READ_PATTERN_DETECT" \
            | sed -E "s/$MDIO_READ_PATTERN_EXTRACT/\1/")
        val=$(( (result >> 11) & 0x1 ))
        case "$val" in
            0)
                link_state="Link Down"
                ;;
            1)
                link_state="Link Up  "
                ;;
        esac
        printf "%sPort: %s - %s\n" "$tab" "$port" "$link_state"
    done
    tab=${tab:1}
}

do_read_spdsts() {
    local result ports port val
    ports="$*"

    printf "%sLink speed:\n" "$tab"
    tab="	$tab"

    for port in $ports; do
        port=${port^^}
        result=$(oob-mdio-util.sh read "$port" 0x0 \
            | grep -e "$MDIO_READ_PATTERN_DETECT" \
            | sed -E "s/$MDIO_READ_PATTERN_EXTRACT/\1/")
        val=$(( (result >> 10) & 0x1 ))
        case "$val" in
            0)
                duplex="half-duplex"
                ;;
            1)
                duplex="full-duplex"
                ;;
        esac
        
        val=$(( (result >> 8) & 0x3 ))
        case "$val" in
            0)
                speed="10   Mb/s"
                ;;
            1)
                speed="100  Mb/s"
                ;;
            2)
                speed="1000 Mb/s"
                ;;
        esac
        printf "%sPort: %s - %s %s \n" "$tab" "$port" "$speed" "$duplex"
    done
    tab=${tab:1}
}

read_lnksts() {
    ports=$(check_sts_args "$@")
    do_read_lnksts "$ports"
}

read_spdsts() {
    ports=$(check_sts_args "$@")
    do_read_spdsts "$ports"
}

read_status() {
    ports=$(check_sts_args "$@")
    printf "%sStatus:\n" "$tab"
    tab="	$tab"
    do_read_lnksts "$ports"
    do_read_spdsts "$ports"
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
