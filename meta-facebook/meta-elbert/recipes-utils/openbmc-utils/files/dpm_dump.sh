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
source /usr/local/bin/dpm_utils.sh
source /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

logfile="/mnt/data1/log/dpm_log"
logfile_old="${logfile}.1"
max_loglines=1000
switchcard_bus=3

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
read_logs "$switchcard_bus" "SWITCHCARD" >> "$logfile"

# The UCD90160B does not support a security bitmask. Disable
# security in order to update the clock. Security will be
# re-enabled later in the boot.
retry_command 3 disable_power_security_mode 3 0x4e "UCD90160B"
update_clock "$switchcard_bus"
# For the switchcard UCD90160B, there are 18 bytes to clear
echo "Clearing SWITCHCARD DPM logs..."
i2cset -y 3 0x4e 0xea 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 s
