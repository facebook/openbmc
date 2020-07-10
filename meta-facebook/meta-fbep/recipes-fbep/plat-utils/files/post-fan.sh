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
# Provides:          post-fan
# Required-Start:
# Required-Stop:
# Default-Start:
# Default-Stop:      6
# Short-Description: Set fan speed before reboot
### END INIT INFO

vendor=$(/usr/local/bin/cfg-util asic_manf)
if [[ "$vendor" == "AMD" ]]; then
  PWM=70
else
  PWM=100
fi

echo "Setup fan speed to $PWM%"
/usr/local/bin/fan-util --set $PWM
