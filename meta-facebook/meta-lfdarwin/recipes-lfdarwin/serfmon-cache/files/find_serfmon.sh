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
### BEGIN INIT INFO
# Provides:          find_serfmon.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Find Serfmon string from OBMC log and cache it
# Based on: meta-facebook/meta-fbdarwin/recipes-utils/serfmon-cache/files/find_serfmon.sh
### END INIT INFO

OBMC_LOGFILE="/var/log/obmc-console-host0.log"
SERFMON_CACHE="/mnt/data/userver_serial_number.txt"
MACMON_CACHE="/mnt/data/userver_mac_address.txt"
SERFMON_REGEX="\!serfmon\:.*\:.*\:.*$"
MACMON_REGEX="\!macmon\:.*\:.*\:.*$"

maybe_update_cache() {
    # Check the OBMC logfile for the given regex. If found, update the cache_file
    local -n cached_value="$1"
    local -n cache_updated="$2"
    regex="$3"
    cache_file="$4"
    name="$5"

    # Always clear update flag first
    cache_updated=0

    if grep -qE "$regex" "$OBMC_LOGFILE"; then
        # Detect if string found in OBMC log
        read_value="$(grep -oE "$regex" "$OBMC_LOGFILE" | tail -n 1)"

        # Read cached string
        if [ -f "$cache_file" ]; then
            cached_value="$(cat "$cache_file")"
        else
            cached_value=""
        fi

        # If OBMC's string value doesn't match the cache, update it
        if [ "$read_value" != "$cached_value" ]; then
            value_to_write="$(echo "$read_value" | awk -F":" '{print $3}')"
            echo "$value_to_write" > "$cache_file"
            echo "Registered new $name string: $value_to_write"
            cached_value="$value_to_write"
            # shellcheck disable=SC2034
            cache_updated=1
        fi
    fi
}

# Print cached Serfmon/Macmon strings on starting service
cached_serfmon=""
if [ -f "$SERFMON_CACHE" ]; then
    cached_serfmon="$(cat "$SERFMON_CACHE")"
    echo "Serial Number: $cached_serfmon"
fi

cached_macmon=""
if [ -f "$MACMON_CACHE" ]; then
    cached_macmon="$(cat "$MACMON_CACHE")"
    echo "MAC Address: $cached_macmon"
fi

serfmon_updated=0
macmon_updated=0
while true; do
    if [ -f "$OBMC_LOGFILE" ]; then
        maybe_update_cache cached_serfmon serfmon_updated "$SERFMON_REGEX" \
            "$SERFMON_CACHE" "Serial Number"
        maybe_update_cache cached_macmon macmon_updated "$MACMON_REGEX" \
            "$MACMON_CACHE" "MAC Address"

        if [ "$serfmon_updated" != 0 ] || [ "$macmon_updated" != 0 ]; then
            # TODO: supply this to the appropiate DBUS interface somehow
            echo "Updated cache files."
        fi
    fi
    sleep 15
done
