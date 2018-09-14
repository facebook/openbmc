#!/bin/sh
#
# Copyright 2017-present Facebook. All Rights Reserved.
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


SENSOR_UTIL="/usr/local/bin/sensor-util"
SENSORDUMP_FILE="/mnt/data/sensordump"

logger -t "gpiointrd" -p daemon.crit "Sensordump is generated at $SENSORDUMP_FILE"

echo "Sensor dump log created at " > $SENSORDUMP_FILE  && date >> $SENSORDUMP_FILE && echo $'\n\n' >> $SENSORDUMP_FILE && echo "Senor Dump" >> $SENSORDUMP_FILE && $SENSOR_UTIL all >> $SENSORDUMP_FILE
exit 0
