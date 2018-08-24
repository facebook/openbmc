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
# Provides:          setup-fan
# Required-Start:    board-id
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set fan speed
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

default_fsc_config_path="/etc/fsc-config.json"
sku_type=`cat /tmp/slot.bin`
full_config=1

echo "Setup fan speed... "

#for i in `seq 1 1 4`
#do
#   if [ $(is_server_prsnt $i) == "0" ]; then
#     echo "slot$i is empty"
#     full_config=0
#   fi
#done

#if [ $full_config -eq 0 ]; then
#   echo "Enter into transitional mode - Unexpected sku type (system is not full config)!"
#   /usr/local/bin/init_pwm.sh
#   /usr/local/bin/fan-util --set 70
#   exit 1
#fi

case "$sku_type" in
   "0")
       echo "Run FSC 4 TLs Config"
       cp /etc/FSC_MINILAKETB_DVT.json ${default_fsc_config_path}
   ;;
   *)  echo "Unexpected sku type! Use default config"
       cp /etc/FSC_MINILAKETB_DVT.json ${default_fsc_config_path}
   ;;
esac

/usr/local/bin/init_pwm.sh
/usr/local/bin/fan-util --set 50
runsv /etc/sv/fscd > /dev/null 2>&1 &

# Check SLED in/out
if [ $(gpio_get H5) = 1 ]; then
   sv stop fscd
   /usr/local/bin/fan-util --set 100
fi

echo "done."
