#!/bin/bash
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
# shellcheck disable=SC2034
source /usr/local/bin/dpm_utils.sh

prog="$0"
usage() {
    echo "Usage: $prog [--clear] [--verbose ] (2-9)"
    echo
    echo "options:"
    echo " --clear: clear faults after dumping them"
    echo " --verbose: print debug messages to stderr"
    echo "(2-9): PIM number"
}

clear_faults="no"
while [ $# -gt 0 ]
do
    case "$1" in
        --clear)
            clear_faults="yes"
            ;;
        --verbose)
            dpm_verbose="yes"
            ;;
        *)
            break
            ;;
    esac
    shift
done

if [ $# -lt 1 ]
then
    echo "Error: missing PIM number" >&2
    usage
    exit 1
fi

pim_num="$1"
if [ ! "${pim_num##*[!0-9]*}" ]
then
    echo "Error: invalid PIM number" >&2
    usage
    exit 1
fi
if [ "$pim_num" -lt 2 ] || [ "$pim_num" -gt 9 ]
then
    echo "Error: invalid PIM number" >&2
    usage
    exit 1
fi
pim_idx="$((pim_num - 2))"

pim_buses=(16 17 18 19 20 21 22 23)
# P1 has a different PIM bus mapping, but the
# wedge_is_smb_p1 function is not available at
# the time this script runs.
pim_bus="${pim_buses[$pim_idx]}"
fault_source="PIM$pim_num"

clear_faults() {
    bus="$pim_bus"

    update_clock "$bus"

    misc_config="$(i2cget -y "$bus" 0x4e 0xfc i | tail -n 1)"
    config_byte="$((0x${misc_config:2:2}))"
    b2="$((0x${misc_config:4:2}))"
    b3="$((0x${misc_config:6:2}))"
    b4="$((0x${misc_config:8:2}))"

    # In order for faults to be cleared in non-volatile store, the
    # brownout bit in the misc_config byte needs to be cleared.
    no_brown=$((config_byte & ~0x1))

    print_debug "$fault_source: clearing the brownout bit..."
    i2cset -y "$bus" 0x4e 0xfc "$no_brown" "$b2" "$b3" "$b4" s

    print_debug "Clearing $fault_source DPM logs..."
    i2cset -y "$bus" 0x4e 0xea 0 0 0 0 0 0 0 0 0 0 0 0 s

    print_debug "$fault_source: restoring the brownout bit..."
    i2cset -y "$bus" 0x4e 0xfc "$config_byte" "$b2" "$b3" "$b4" s
}

read_logs "$pim_bus" "$fault_source"

if [ "$clear_faults" = "yes" ]
then
    clear_faults
fi
