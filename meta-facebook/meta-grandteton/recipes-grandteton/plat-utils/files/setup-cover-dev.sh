#!/bin/sh
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

rebind_i2c_dev() {
  dev="$1-00$2"
  dri=$3

  if [ ! -L "${SYSFS_I2C_DEVICES}/$dev/driver" ]; then
    if i2c_bind_driver "$dri" "$dev" >/dev/null; then
      echo "rebind $dev to driver $dri successfully"
    fi
  fi
}

echo "Probe VR Device"
#CPU VR
i2c_device_add 20 0x60 isl69260
i2c_device_add 20 0x61 isl69260
i2c_device_add 20 0x63 isl69260
i2c_device_add 20 0x72 isl69260
i2c_device_add 20 0x74 isl69260
i2c_device_add 20 0x76 isl69260
