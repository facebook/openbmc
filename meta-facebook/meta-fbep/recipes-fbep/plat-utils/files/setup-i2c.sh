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

. /usr/local/bin/openbmc-utils.sh

# Keep "power-util mb on" for debug purpose
echo "Waiting for power up"
pwr_ctrl=$(gpio_get PWR_CTRL keepdirection)
for retry in {1..60};
do
  pwr_ready=$(gpio_get SYS_PWR_READY keepdirection)
  if [[ "$pwr_ready" == "1" ]]; then
    break
  elif [[ "$pwr_ctrl" == "1" ]]; then
    power-util mb on > /dev/null
  else
    sleep 1
  fi
done

# Thermal sensors for PCIe switches
i2c_device_add 6 0x4d tmp422
i2c_device_add 6 0x4e tmp422

# Voltage regulators
i2c_bind_driver mpq8645p 5-0030
i2c_bind_driver mpq8645p 5-0031
i2c_bind_driver mpq8645p 5-0032
i2c_bind_driver mpq8645p 5-0033
i2c_bind_driver mpq8645p 5-0034
i2c_bind_driver mpq8645p 5-0035
i2c_bind_driver mpq8645p 5-0036
i2c_bind_driver mpq8645p 5-003b

# P48V HSCs
modprobe adm1275

# i2c mux in front of OAMs
i2c_mux_add_sync 11 0x70 pca9543 21
i2c_mux_add_sync 10 0x70 pca9543 23
i2c_mux_add_sync 9 0x70 pca9543 25
i2c_mux_add_sync 8 0x70 pca9543 27
