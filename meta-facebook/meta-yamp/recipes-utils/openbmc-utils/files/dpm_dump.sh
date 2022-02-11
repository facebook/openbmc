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

dpm_path="/sys/bus/i2c/drivers/ucd9000/3-004e"
logfile="/mnt/data1/log/dpm_log"
logfile_old="${logfile}.1"
max_loglines=1000

trap cleanup INT TERM QUIT EXIT

REBIND=false

if [ -d "$dpm_path" ]; then
    REBIND=true
    echo "# Unbinding SWITCHCARD DPM from ucd9000 driver"
    echo "3-004e" > /sys/bus/i2c/drivers/ucd9000/unbind
fi

# Check to see if the log file needs to be rotated
touch $logfile $logfile_old
lines="$(wc -l $logfile|cut -d' ' -f1)"
if [ "$lines" -gt $max_loglines ]
then
    echo "Rotating DPM log file"
    rm -f "$logfile_old"
    cp "$logfile" "$logfile_old"
    rm -f "$logfile"
fi

echo "Reading DPM logs..."

log=''
log="$log{'Date':'$(date -u "+%Y-%m-%d %T")'"
log="$log,'Log':'$(i2cget -y 3 0x4e 0xea i | tail -n 1)'"

eb="$(i2cget -y 3 0x4e 0xeb w)"

len="${eb:2:2}"

for (( i = 0 ; i < "$len" ; i++ ))
do
  i2cset -y 3 0x4e 0xeb 0x"$len""$(printf '%02x' "$i")" w
  man="$(i2cget -y 3 0x4e 0xec i | tail -n 1)"
  exp="$(i2cget -y 3 0x4e 0x20 | tail -n 1)"
  log="$log,'detail_$i':{'m':'$man','e':'$exp'}"
done

log="$log}"

echo "$log" >> "$logfile"

echo "Clearing DPM logs..."
i2cset -y 3 0x4e 0xea 0 0 0 0 0 0 0 0 0 0 0 0 0 0 s

cleanup() {
    if [ "$REBIND" = true ]; then
        echo "# Re-Binding SWITCHCARD DPM to ucd9000 driver"
        echo "3-004e" > /sys/bus/i2c/drivers/ucd9000/bind
    fi
}
