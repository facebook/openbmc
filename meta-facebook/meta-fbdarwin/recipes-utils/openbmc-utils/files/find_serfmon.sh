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
# Short-Description:  Find Serfmon string from mTerm log and cache it
### END INIT INFO

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

MTERM_LOGFILE="/var/log/mTerm_wedge.log"
SERFMON_CACHE="/mnt/data1/.serfmon.txt"
CHASSIS_EEPROM_CACHE="/mnt/data1/.chassis_eeprom"
SERFMON_REGEX="\!serfmon\:.*\:.*\:.*$"


create_dummy_chassis_eeprom() {
    dummy_pca="PCA000000000"
    # Extract SN from Serfmon Format \!serfmon\:.*\:.*\:.*$
    chassis_serial="$(awk -F":" '{print $3}' "$SERFMON_CACHE")"
    # Create Fake EEPROM file for weutil in expected format
    printf "\x00\x00\x00\x03\x00\x00\x00\x00\x30\x30\x30\x32%s%s"\
        "$dummy_pca" "$chassis_serial" > "$CHASSIS_EEPROM_CACHE"
    # Pad file with all 0xFFs to size
    dd if=/dev/zero bs=1 count=220 \
       2>/dev/null | tr "\000" "\377" >> "$CHASSIS_EEPROM_CACHE"
}

cached_serfmon=""
# Print Cached Serfmon string on starting service
if [ -f "$SERFMON_CACHE" ]; then
    cached_serfmon="$(cat "$SERFMON_CACHE")"
    echo "$cached_serfmon"
    create_dummy_chassis_eeprom
fi

while true; do
    if [ -f "$MTERM_LOGFILE" ]; then
      if grep -q "$SERFMON_REGEX" "$MTERM_LOGFILE"; then
            # Detect if Serfmon string found in mTerm log
            read_serfmon="$(grep -o "$SERFMON_REGEX" "$MTERM_LOGFILE" | tail -n 1)"

            # Read cached Serfmon string from eMMC
            if [ -f "$SERFMON_CACHE" ]; then
                cached_serfmon="$(cat "$SERFMON_CACHE")"
            else
                cached_serfmon=""
            fi

            # If mTerm's serfmon doesn't match the cache, update it
            if [ "$read_serfmon" != "$cached_serfmon" ]; then
                echo "$read_serfmon" > "$SERFMON_CACHE"
                echo "Registered new Serfmon string!"
                cached_serfmon="$(cat "$SERFMON_CACHE")"
                echo "$read_serfmon"
                create_dummy_chassis_eeprom
            fi
        fi
    fi

  # Sleep 15 seconds until the next loop.
  # reduce for faster scan.
  sleep 15
done
