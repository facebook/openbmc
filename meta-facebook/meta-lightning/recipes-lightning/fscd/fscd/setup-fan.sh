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
. /usr/bin/kv

# Get SSD type and vender_device ID from cache
default_fsc_config_path="/etc/fsc-config.json"

# get from cache
flash_type=$(kv_get "ssd_sku_info")
ssd_vendor=$(kv_get "ssd_vendor")

case "$flash_type" in
   "U2")
        case "$ssd_vendor" in
           "intel") cp /etc/FSC_Lightning_PVT_Intel_U2_4TB_v3_config.json ${default_fsc_config_path}
           ;;
           "samsung") cp /etc/FSC_Lightning_PVT_Samsung_U2_4TB_v3_config.json ${default_fsc_config_path}
           ;;
           *) echo "Enter into transitional mode - Unexpected U.2 SSD vendor."
              /usr/local/bin/init_pwm.sh
              /usr/local/bin/fan-util --set 70
              exit 1
           ;;
        esac
   ;;
   "M2")
        case "$ssd_vendor" in
           "seagate") cp /etc/FSC_Lightning_PVT_Seagate_M2_2TB_v4_config.json ${default_fsc_config_path}
           ;;
           "samsung") cp /etc/FSC_Lightning_PVT_Samsung_M2_2TB_v4_config.json ${default_fsc_config_path}
           ;;
           "toshiba") cp /etc/FSC_Lightning_PVT_Toshiba_M2_2TB_v1_config.json ${default_fsc_config_path}
           ;;
           *) echo "Enter into transitional mode - Unexpected M.2 SSD vendor."
              /usr/local/bin/init_pwm.sh
              /usr/local/bin/fan-util --set 70
              exit 1
           ;;
         esac
   ;;
   *) echo "Enter into transitional mode - Unexpected flash type!"
      /usr/local/bin/init_pwm.sh
      /usr/local/bin/fan-util --set 70
      exit 1
   ;;
esac


echo -n "Setup fan speed... "
/usr/local/bin/init_pwm.sh
/usr/local/bin/fan-util --set 50
runsv /etc/sv/fscd > /dev/null 2>&1 &
echo "done."
