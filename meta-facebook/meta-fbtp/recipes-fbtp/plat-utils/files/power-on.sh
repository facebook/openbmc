#!/bin/sh
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

### BEGIN INIT INFO
# Provides:          power-on
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Power on Server
### END INIT INFO
. /usr/local/fbpackages/utils/ast-functions
KV_STORE="/mnt/data/kv_store"

pwr_on_host() {
  pwr_sts=$(gpio_get PWRGD_SYS_PWROK)
  if [ "$pwr_sts" != "1" ]; then
    gpio_set FM_BMC_PWRBTN_OUT_N 0
    sleep 1
    gpio_set FM_BMC_PWRBTN_OUT_N 1
    sleep 1
    pwr_sts=$(gpio_get PWRGD_SYS_PWROK)
    if [ "$pwr_sts" == "1" ]; then
      logger "Successfully power on the server"
    fi
  else
    logger "The server was powered on"
  fi
}

check_por_config() {
  #0:Power on reset 1:first power on
  echo "$(cat /tmp/ast_por)"
}

echo -n "Power on the server..."

#get por cfg
por_conf="$KV_STORE/server_por_cfg"
lps_conf="$KV_STORE/pwr_server_last_state"
if ! [ -f $por_conf ]; then
    # power on the server directly
    # healthd is S91 and power-on.sh is S66 in rc5.d
    # when the system is first powered on, BMC would power on the server directly
    logger -s -p user.info -t power-on "Power Policy can't be found, power on the server directly"
    pwr_on_host
else
    # control the system power based on $por_conf
    por_val=$(cat $por_conf)
    lps_val=$(cat $lps_conf)
    logger -s -p user.info -t power-on "Power Policy: $por_val, Last Power State: $lps_val"
    if [ "$por_val" == "on" ]; then
      pwr_on_host
    elif [ "$por_val" == "lps" ] && [ "$lps_val" == "on" ]; then
      pwr_on_host
    fi
fi

echo "Done"

echo "Check if P12V_STBY was turned off before..."

check_log=$($LOGUTIL_CMD all --print | grep -v grep | grep "Turn off P12V_STBY" | tail -n 1)

if [ -n "$check_log" ]; then
   logger -s -p user.crit -t power-on "P12V_STBY previously disabled!"
   #logger -s -p user.crit -t power-on "Turn off P12V_STBY because it was turned off before"
   #turn_off_p12v_stby
fi

echo "Done"
