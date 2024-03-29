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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
#shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

#
# Will need to move some devices to dts later.
#
i2c_device_add 8 0x50 24c04
i2c_device_add 8 0x54 24c64

# Setup Chassis/PDB IDPROM
setup_chassis_idprom() {
    idp_path=$(i2cdetect -l | grep SCM_IDPROM)
    if [ -n "${idp_path}" ]; then
        bus=$(echo "${idp_path}" | tr "-" " " | awk '{print $2}')
        i2c_device_add "${bus}" 0xa0d0 24c64 #Chassis IDPROM
    fi
}

setup_chassis_idprom
