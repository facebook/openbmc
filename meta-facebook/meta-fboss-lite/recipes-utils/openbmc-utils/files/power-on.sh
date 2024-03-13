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

### BEGIN INIT INFO
# Provides:          power-on
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Power on micro-server
### END INIT INFO

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

do_power_on() {
    retry=0
    max_retry=5

    while [ "$retry" -lt "$max_retry" ]; do
        logger -p user.crit "Power on uServer (from power-on.sh).."

        echo "Power on uServer.."
        if userver_power_on; then
            sleep 1
            if userver_power_is_on; then
                echo "uServer is powered on successfully."
                return 0
            fi
        fi

        retry=$((retry+1))
        echo "Error: failed to power on uServer! Retry ($retry / $max_retry).."
        sleep 1
    done
}

echo "Checking x86 (userver) power status... "
if userver_power_is_on; then
    echo "userver is already powered. Skipped."
    exit 0
fi

do_power_on
