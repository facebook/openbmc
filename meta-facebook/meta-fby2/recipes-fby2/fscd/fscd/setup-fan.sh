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
sku_type=0
server_type=0
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

for i in `seq 1 1 4`
do
  tmp_sku=$(get_slot_type $i)
  sku_type=$(($(($tmp_sku << $(($(($i*4)) - 4))))+$sku_type))
  tmp_server=$(get_server_type $i)
  server_type=$(($(($tmp_server << $(($(($i*2)) - 2))))+$server_type))
done

case "$sku_type" in
   "0")
     case "$server_type" in
       "0")
         echo "Run FSC 4 TLs Config"
         cp /etc/FSC_FBY2_PVT_4TL_config.json ${default_fsc_config_path}
       ;;
       "85")
         echo "Run FSC 4 RCs Config"
         cp /etc/FSC_FBRC_DVT_4RC_config.json ${default_fsc_config_path}
       ;;
       "170")
         echo "Run FSC 4 EPs Config"
         cp /etc/FSC_FBEP_DVT_4EP_config.json ${default_fsc_config_path}
       ;;
       *)
         echo "Unexpected 4 Servers config! Run FSC 4 TLs Config as default config"
         cp /etc/FSC_FBY2_PVT_4TL_config.json ${default_fsc_config_path}
       ;;
     esac
   ;;
   "514")
     if [[ $(get_server_type 2) == "0" && $(get_server_type 4) == "0" ]] ; then
       echo "Run FSC 2 GPs and 2 TLs Config"
       cp /etc/FSC_FBY2_PVT_2GP_2TL_config.json ${default_fsc_config_path}
     elif [[ $(get_server_type 2) == "1" && $(get_server_type 4) == "1" ]] ; then
       echo "Run FSC 4 RCs Config"
       cp /etc/FSC_FBRC_DVT_4RC_config.json ${default_fsc_config_path}
     elif [[ $(get_server_type 2) == "2" && $(get_server_type 4) == "2" ]] ; then
       echo "Run FSC 2 GPs and 2 EPs Config"
       cp /etc/FSC_FBEP_DVT_2GP_2EP_config.json ${default_fsc_config_path}
     else
       echo "Unexpected 2 GPs and 2 Servers config! Run FSC 2 GPs and 2 TLs Config as default config"
       cp /etc/FSC_FBY2_PVT_2GP_2TL_config.json ${default_fsc_config_path}
     fi
   ;;
   "257")
     if [[ $(get_server_type 2) == "0" && $(get_server_type 4) == "0" ]] ; then
       echo "Run FSC 2 CFs and 2 TLs Config"
       cp /etc/FSC_FBY2_PVT_2CF_2TL_config.json ${default_fsc_config_path}
     elif [[ $(get_server_type 2) == "1" && $(get_server_type 4) == "1" ]] ; then
       echo "Run FSC 4 RCs Config"
       cp /etc/FSC_FBRC_DVT_4RC_config.json ${default_fsc_config_path}
     elif [[ $(get_server_type 2) == "2" && $(get_server_type 4) == "2" ]] ; then
       echo "Run FSC 4 EPs Config"
       cp /etc/FSC_FBEP_DVT_4EP_config.json ${default_fsc_config_path}
     else
       echo "Unexpected 2 CFs and 2 Servers config! Run FSC 2 CFs and 2 TLs Config as default config"
       cp /etc/FSC_FBY2_PVT_2CF_2TL_config.json ${default_fsc_config_path}
     fi
   ;;
   "1028")
     echo "Run FSC 2 GPV2s and 2 TLs Config"
     cp /etc/FSC_FBGPV2_EVT_config.json ${default_fsc_config_path}
   ;;
   *)
     server_type_tmp="3"
     for i in 1 2 3 4 ; do
       server_type_tmp=$(get_server_type $i)
       if [ "$server_type_tmp" != "3" ] ; then
         break;
       fi
     done

     if [ "$server_type_tmp" == "1" ] ; then
       echo "Unexpected sku type! Use FSC 4 RCs Config as default config"
       cp /etc/FSC_FBRC_DVT_4RC_config.json ${default_fsc_config_path}
     elif [ "$server_type_tmp" == "2" ] ; then
       echo "Unexpected sku type! Use FSC 4 EPs Config as default config"
       cp /etc/FSC_FBEP_DVT_4EP_config.json ${default_fsc_config_path}
     else
       echo "Unexpected sku type! Use FSC 4 TLs Config as default config"
       cp /etc/FSC_FBY2_PVT_4TL_config.json ${default_fsc_config_path}
     fi
   ;;
esac

/usr/local/bin/init_pwm.sh
/usr/local/bin/fan-util --set 50
runsv /etc/sv/fscd > /dev/null 2>&1 &
logger -p user.info "fscd started"

# Check SLED in/out
if [ $(gpio_get H5) = 1 ]; then
   logger -p user.warning "SLED not seated, fscd stopped, set fan speed to 100%"
   sv stop fscd
   /usr/local/bin/fan-util --set 100
fi

echo "done."
