#! /bin/sh
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
# Provides:          server-init
# Required-Start:
# Required-Stop:
# Default-Start:     
# Default-Stop:      
# Short-Description: Provides a service to reinitialize the server
#
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

slot_num=$1
PID=$$
PID_FILE="/var/run/server-init.${slot_num}.pid"
bus=$((slot_num-1))

# check if the script is running
[ -r $PID_FILE ] && OLDPID=`cat $PID_FILE` || OLDPID=''

# Set the current pid
echo $PID > $PID_FILE

# kill the previous running script if it is existed
if [ ! -z "$OLDPID" ] && (grep "server-init" /proc/$OLDPID/cmdline &> /dev/null) ; then
  echo "kill pid $OLDPID..."
  kill -s 9 $OLDPID
  ps | grep 'bic-cached' | grep "slot${slot_num}" | awk '{print $1}'| xargs kill -s 9 &> /dev/null
  ps | grep 'power-util' | grep "slot${slot_num}" | awk '{print $1}'| xargs kill -s 9 &> /dev/null
fi
unset OLDPID

if [ $(is_server_prsnt $slot_num) == "0" ]; then
  /usr/bin/sv stop ipmbd_${bus}
  disable_server_12V_power $slot_num
  gpio_set FM_BMC_SLOT${bus}_ISOLATED_EN_R 0
  rm -f /tmp/*fruid_slot${slot_num}*
  rm -f /tmp/*sdr_slot${slot_num}*
else
  enable_server_12V_power $slot_num
  sleep 1 #wait for power is ready
  enable_server_i2c_bus $slot_num
  sleep 1 #wait for bus is ready
  /usr/bin/sv start ipmbd_${bus}
  sleep 2 #wait for ipmbd is ready  
  /usr/local/bin/power-util slot${slot_num} on
  /usr/local/bin/bic-cached -s slot${slot_num}
  /usr/local/bin/bic-cached -f slot${slot_num}
fi

# restart the service
sv restart gpiod
sv restart sensord

