#!/bin/bash
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

. /usr/local/fbpackages/utils/ast-functions

for retry in {1..20};
do
    bp2_sensor238=$(kv get fan_bp2_sensor238)
    if [ $? -ne 0 ]; then
        sleep 3
    else
        break
    fi
done

i2cget -f -y 8 0x48 0x01
rev=$?
if [ "$rev" -ne 0 ]; then
    val=$(i2cget -f -y 40 0x21 0x00)
    fan_present1=$(("$val"))

    val=$(i2cget -f -y 41 0x21 0x00)
    fan_present2=$(("$val"))

    if [ "$fan_present1" -eq 255 ] && [ "$fan_present2" -eq 255 ]; then
      echo "Don't enable fscd due to fan not present"
    else
      echo "Start fscd"
      #runsv /etc/sv/fscd > /dev/null 2>&1 &
      fan-util --set 50
    fi
fi
echo "done."
