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

dpm_verbose="no"
print_debug() {
    if [ "$dpm_verbose" = "yes" ]
    then
        echo "$*" >&2
    fi
}

read_logs() {
    local bus="$1"
    local fault_source="$2"
    dpm_path="/sys/bus/i2c/drivers/ucd9000/$bus-004e"

    if [ -d "$dpm_path" ]; then
        echo "Script cannot run while driver present" >&2
        echo "Use the following commands to unbind/rebind" >&2
        echo 'echo "'"$bus"'-004e" > /sys/bus/i2c/drivers/ucd9000/unbind' >&2
        echo 'echo "'"$bus"'-004e" > /sys/bus/i2c/drivers/ucd9000/bind' >&2
        exit 1
    fi

    print_debug "Reading $fault_source DPM logs..."

    echo -n "{'Date':'$(date -u "+%Y-%m-%d %T")'"
    echo -n ",'Source':'$fault_source'"
    echo -n ",'Log':'$(i2cget -y "$bus" 0x4e 0xea s)'"

    eb="$(i2cget -y "$bus" 0x4e 0xeb w)"

    len="$((0x${eb:2:2}))"

    for (( i = 0 ; i < "$len" ; i++ ))
    do
      i2cset -y "$bus" 0x4e 0xeb "0x$len$(printf '%02x' "$i")" w
      man="$(i2cget -y "$bus" 0x4e 0xec s)"
      exp="$(i2cget -y "$bus" 0x4e 0x20 | tail -n 1)"
      echo -n ",'detail_$i':{'m':'$man','e':'$exp'}"
    done

    echo "}"
}

update_clock() {
    local bus="$1"

    # Make sure the time set set correctly on the DPM
    seconds="$(date +%s)"
    seconds_per_day="$((24 * 60 * 60))"
    days="$((seconds / seconds_per_day))"
    milliseconds="$((seconds % seconds_per_day * 1000))"
    d1="$((days >> 24))"
    d2="$((days >> 16 & 0xff))"
    d3="$((days >> 8 & 0xff))"
    d4="$((days & 0xff))"
    m1="$((milliseconds >> 24))"
    m2="$((milliseconds >> 16 & 0xff))"
    m3="$((milliseconds >> 8 & 0xff))"
    m4="$((milliseconds & 0xff))"

    print_debug "$fault_source: updating run time clock on DPM"
    i2cset -y "$bus" 0x4e 0xd7 "$m1" "$m2" "$m3" "$m4" "$d1" "$d2" "$d3" "$d4" s
}
