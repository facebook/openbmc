#!/bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
# Default-Start:     5
# Default-Stop:
# Short-Description: Set fan speed
### END INIT INFO

# shellcheck disable=SC1091
. /usr/local/fbpackages/utils/ast-functions


default_fsc_config="/etc/fsc-config.json"
if [[ -f ${default_fsc_config} ]]; then
  rm ${default_fsc_config}
fi

rev_id=$(kv get mb_rev)
if [[ "$rev_id" == "1" ]]; then
    ln -s /etc/fsc-config-8-retimer.json ${default_fsc_config}
elif [[ "$rev_id" == "2" ]]; then
    ln -s /etc/fsc-config-2-retimer.json ${default_fsc_config}
else
    ln -s /etc/fsc-config-evt.json ${default_fsc_config}
fi

for retry in {1..3};
do
    bp1_sensor208=$(kv get fan_bp1_sensor208)
    if [ $? -ne 0 ]; then
        sleep 3
    else
        break
    fi
done

for retry in {1..3};
do
    bp1_sensor208=$(kv get fan_bp1_sensor208)
    if [ $? -ne 0 ]; then
        sleep 3
    else
        break
    fi
done

runsv /etc/sv/fscd > /dev/null 2>&1 &
echo "done."
