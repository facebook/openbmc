#!/bin/bash
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
# Provides:          setup_sensors_conf.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Setup lm-sensors config
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

SENSORS_CONF_PATH="/etc/sensors.d"
MUX_BUS=7
MUX_ADDR=0x70

# $1: Mux register value
set_mux_channel() {
  val=$1

  i2cset -f -y "$MUX_BUS" "$MUX_ADDR" "$val" 2> /dev/null
}

# $1: FRU eeprom address
get_fru_version() {
  addr=$1

  i2cset -f -y "$MUX_BUS" "$addr" 0x00 2> /dev/null
  usleep 50000
  i2cset -f -y "$MUX_BUS" "$addr" 0x4D 2> /dev/null
  usleep 50000
  output=$(i2cget -f -y "$MUX_BUS" "$addr" 2> /dev/null)
  ret=$?
  if [ "$ret" -eq 0 ]; then
    echo "$output"
  else
    echo "-1"
  fi

}

set_mux_channel 1
fru1_version=$(get_fru_version 0x56)
set_mux_channel 2
fru2_version=$(get_fru_version 0x55)

# Config LTC4151 Rsense value at different hardware version.
# Kernel driver use Rsense equal to 1 milliohm. We need to correct Rsense
# value, and all value are from hardware team.
if [ "$((fru1_version))" -ge "$((0x02))" ] || \
   [ "$((fru2_version))" -ge "$((0x02))" ]; then
    ltc4151_rsense=1.55
else
    ltc4151_rsense=1.35
fi

sed "s/rsense/$ltc4151_rsense/g" "$SENSORS_CONF_PATH"/custom/ltc4151.conf \
> "$SENSORS_CONF_PATH"/ltc4151.conf
