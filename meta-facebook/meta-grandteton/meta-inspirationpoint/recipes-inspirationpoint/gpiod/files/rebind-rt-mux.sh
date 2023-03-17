#!/bin/sh
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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

. /usr/local/bin/openbmc-utils.sh

RETRY=0

rebind_i2c_dev() {
  dev="$1-00$2"
  dri=$3

  while [ ! -L "${SYSFS_I2C_DEVICES}/$dev/driver" ] && [ $RETRY -lt 10 ];
  do
    sleep 1
    i2c_bind_driver "$dri" "$dev"
    ((RETRY++))
  done
}

rebind_i2c_dev 6 70 pca954x
