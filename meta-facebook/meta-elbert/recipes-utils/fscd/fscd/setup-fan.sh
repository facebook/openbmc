#!/bin/sh
#
# Copyright 2020-present Facebook. All Rights Reserved.
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
# Provides:          setup-fan
# Required-Start:    board-id
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set fan speed
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

if ! wedge_is_bmc_personality; then
    echo "uServer is not in BMC personality. Skipping all fan control"
    return
fi

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh
default_fsc_config="/etc/fsc-config.json"

if ! wedge_is_bmc_personality; then
    echo "uServer is not in BMC personality. Skipping all fan control"
    return
fi

num_pim16q=0
num_pim8ddm=0

pim_list="2 3 4 5 6 7 8 9"
for pim in ${pim_list}; do
     fru="$(peutil "$pim" 2>&1)"
     if echo "$fru" | grep -q '88-16CD'; then
         num_pim16q=$((num_pim16q+1))
     elif echo "$fru" | grep -q '88-8D'; then
         num_pim8ddm=$((num_pim8ddm+1))
     fi
done

if [ $num_pim16q -eq 8 ]; then
    # 8xPIM16Q
    cp /etc/FSC-8-PIM16Q-config.json ${default_fsc_config}
elif [ $num_pim16q -eq 5 ] && [ $num_pim8ddm -eq 3 ]; then
    cp /etc/FSC-5-PIM16Q-3-PIM8DDM-config.json ${default_fsc_config}
elif [ $num_pim16q -eq 2 ] && [ $num_pim8ddm -eq 6 ]; then
    cp /etc/FSC-2-PIM16Q-6-PIM8DDM-config.json ${default_fsc_config}
else
    # Assume highest power configuration otherwise
    cp /etc/FSC-8-PIM8DDM-config.json ${default_fsc_config}
fi

# Create dummy sensors which host will write
echo 0 > /tmp/.PIM_QSFP200
echo 0 > /tmp/.PIM_QSFP400
echo 0 > /tmp/.PIM_F104

echo "Setup fan speed..."
/usr/local/bin/set_fan_speed.sh 30
runsv /etc/sv/fscd > /dev/null 2>&1 &
echo "done."
