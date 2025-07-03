#!/bin/sh
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
# Provides:          setup-sensord
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Setup sensor monitoring
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

# Call "fw-util mb --version" once before sensor monitoring to store vr information
echo "Get MB FW version... "
/usr/bin/fw-util mb --version > /dev/null

if [ "$(gpio_get FM_BOARD_SKU_ID0)" = "0" ]; then
  ln -s /etc/sensor-correction-sku0-conf.json /etc/sensor-correction-conf.json
else
  new_source=$(/usr/local/bin/fruid-util mb | grep "KSP2907ATF")
  if [ -z "$new_source" ];
  then
    ln -s /etc/sensor-correction-sku1-conf.json /etc/sensor-correction-conf.json
  else
    ln -s /etc/sensor-correction-sku1-diode-conf.json /etc/sensor-correction-conf.json
  fi
fi

echo -n "Setup sensor monitoring for FBTP... "

count=0
while [ 1 ];
do
  ((count++))
  response=$(/usr/local/bin/me-util mb 0xb8 0xd9 0x57 0x01 0x0 0x86 0x8a 0x0 0x0 0x1 0x2 0xd4)
  byte4=$(echo $response | cut -d ' ' -f 4)
  byte5=$(echo $response | cut -d ' ' -f 5)

  if [ "$byte4" = "1E" ] && [ "$byte5" = "3F" ]; then
    logger -t "sensord" -p daemon.info "Read 12v_OUT from ME"
    break
  elif [ $count -ge 30 ]; then
    logger -t "sensord" -p daemon.crit "Fail to enable HSC 12Vout sensor, ME resp: $response"
    break
  else
    # Vout is not enabled, set HSC_0xD4 register to 3F1Eh to enable Vout
    /usr/local/bin/me-util mb 0xb8 0xd9 0x57 0x01 0x0 0x88 0x8a 0x0 0x0 0x3 0x0 0xd4 0x1e 0x3f
    sleep 1
  fi
done

runsv /etc/sv/sensord > /dev/null 2>&1 &

echo "done."
