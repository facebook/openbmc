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
default_aggregate_config_path="/etc/aggregate-sensor-conf.json"
sku_type=0
server_type=0
full_config=1
dev_type=0

DEV_TYPE_UNKNOWN=0
DEV_TYPE_SSD=1
DEV_TYPE_VSI_ACC=2
DEV_TYPE_BRCM_ACC=3
DEV_TYPE_OTHER_ACC=4
DEV_TYPE_DUAL_M2=5

FAN_CONFIG_10K=0
FAN_CONFIG_15K=1

SLOT_TYPE_GPV2=4
has_gpv2=0
invalid_gpv2_config=0

if [ $# -eq 1 ] ; then
  dev_type=$1
fi

echo "Setup fan speed... "

spb_type=$(get_spb_type)

if [ $spb_type == 1 ] ; then
  for i in `seq 1 1 4`
  do
     if [ $(is_server_prsnt $i) == "0" ]; then
       echo "slot$i is empty"
       full_config=0
     fi
  done

  if [ $full_config -eq 0 ]; then
     echo "System is missing a slot - Unexpected sku type (system is not full config)!"
     /usr/local/bin/init_pwm.sh
     /usr/local/bin/fan-util --set 90
     exit 1
  fi
fi

/usr/local/bin/init_pwm.sh
if [ ! -f /tmp/cache_store/setup_fan_config ]; then
  logger -p user.warning "Setup fan config"
  /usr/local/bin/check_fan_config.sh
  echo 1 > /tmp/cache_store/setup_fan_config
fi

fan_config=$(cat /tmp/fan_config)

for i in `seq 1 1 4`
do
  tmp_sku=$(get_slot_type $i)
  if [ "$tmp_sku" == "$SLOT_TYPE_GPV2" ] ; then
    has_gpv2=1
  fi
  sku_type=$(($(($tmp_sku << $(($(($i*4)) - 4))))+$sku_type))
  tmp_server=$(get_server_type $i)
  server_type=$(($(($tmp_server << $(($(($i*4)) - 4))))+$server_type))
done

case "$sku_type" in
   "0")
     case "$server_type" in
       "0")
         echo "Run FSC 4 TLs Config"
         cp /etc/FSC_FBY2_PVT_4TL_config.json ${default_fsc_config_path}
       ;;
       "4369")
         echo "Run FSC 4 RCs Config"
         cp /etc/FSC_FBRC_DVT_4RC_config.json ${default_fsc_config_path}
       ;;
       "8738")
         echo "Run FSC 4 EPs Config"
         cp /etc/FSC_FBEP_DVT_4EP_config.json ${default_fsc_config_path}
       ;;
       "17476")
         echo "Run FSC 4 NDs Config"
         cp /etc/FSC_FBND_DVT_4ND_config.json ${default_fsc_config_path}
         if [ "$fan_config" == "$FAN_CONFIG_15K" ] ; then
            cp /etc/aggregate-sensor-fbnd-conf.json ${default_aggregate_config_path}
         fi
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
     elif [[ $(get_server_type 2) == "4" && $(get_server_type 4) == "4" ]] ; then
       echo "Run FSC 4 NDs Config"
       cp /etc/FSC_FBND_DVT_4ND_config.json ${default_fsc_config_path}
       if [ "$fan_config" == "$FAN_CONFIG_15K" ] ; then
         cp /etc/aggregate-sensor-fbnd-conf.json ${default_aggregate_config_path}
       fi
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
     elif [[ $(get_server_type 2) == "4" && $(get_server_type 4) == "4" ]] ; then
       echo "Run FSC 4 NDs Config"
       cp /etc/FSC_FBND_DVT_4ND_config.json ${default_fsc_config_path}
       if [ "$fan_config" == "$FAN_CONFIG_15K" ] ; then
         cp /etc/aggregate-sensor-fbnd-conf.json ${default_aggregate_config_path}
       fi
     else
       echo "Unexpected 2 CFs and 2 Servers config! Run FSC 2 CFs and 2 TLs Config as default config"
       cp /etc/FSC_FBY2_PVT_2CF_2TL_config.json ${default_fsc_config_path}
     fi
   ;;
   "1028") # 2GPv2 +2TL
     echo "Run FSC 2 GPV2s and 2 TLs Config"
   ;;
   *)
     if [ "$has_gpv2" == "1" ] ; then
       invalid_gpv2_config=1
       echo "Unexpected GPv2 sku type!"
     else
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
      elif [ "$server_type_tmp" == "4" ] ; then
        echo "Unexpected sku type! Use FSC 4 NDs Config as default config"
        cp /etc/FSC_FBND_DVT_4ND_config.json ${default_fsc_config_path}
        if [ "$fan_config" == "$FAN_CONFIG_15K" ] ; then
          cp /etc/aggregate-sensor-fbnd-conf.json ${default_aggregate_config_path}
        fi
      else
        echo "Unexpected sku type! Use FSC 4 TLs Config as default config"
        cp /etc/FSC_FBY2_PVT_4TL_config.json ${default_fsc_config_path}
      fi
     fi
   ;;
esac

if [ "$has_gpv2" == "1" ] ; then
  if [ "$dev_type" == "$DEV_TYPE_UNKNOWN" ] ; then
    fw_ver=$(/usr/bin/fw-util bmc --version fscd)
    if [[ $fw_ver =~ "fbgpv2" ]] ; then
      echo "Keep FSC config : $fw_ver"
      logger -p user.info "Keep FSC config : $fw_ver"
    else
      echo "Run default FSC for M.2 devices"
      logger -p user.info "Run default FSC for M.2 devices"
      if [ "$fan_config" == "$FAN_CONFIG_15K" ] ; then
        cp /etc/FSC_FBGPV2_DVT_config.json ${default_fsc_config_path}
      else
        cp /etc/FSC_FBGPV2_10KFAN_DVT_config.json ${default_fsc_config_path}
      fi
    fi
  elif [ "$dev_type" == "$DEV_TYPE_SSD" ] ; then
    echo "Run FSC for SSD"
    logger -p user.info "Run FSC for SSD"
    if [ "$fan_config" == "$FAN_CONFIG_15K" ] ; then
      cp /etc/FSC_FBGPV2_DVT_config.json ${default_fsc_config_path}
    else
      cp /etc/FSC_FBGPV2_10KFAN_DVT_config.json ${default_fsc_config_path}
    fi
  elif [ "$dev_type" == "$DEV_TYPE_VSI_ACC" ] ; then
    echo "Run FSC for VSI Accelerator"
    logger -p user.info "Run FSC for VSI Accelerator"
    if [ "$fan_config" == "$FAN_CONFIG_15K" ] ; then
      cp /etc/FSC_FBGPV2_VSI_DVT_config.json ${default_fsc_config_path}
    else
      cp /etc/FSC_FBGPV2_VSI_10KFAN_DVT_config.json ${default_fsc_config_path}
    fi
  elif [ "$dev_type" == "$DEV_TYPE_BRCM_ACC" ] ; then
    echo "Run FSC for BRCM Accelerator"
    logger -p user.info "Run FSC for BRCM Accelerator"
    if [ "$fan_config" == "$FAN_CONFIG_15K" ] ; then
      cp /etc/FSC_FBGPV2_BRCM_DVT_config.json ${default_fsc_config_path}
    else
      cp /etc/FSC_FBGPV2_BRCM_10KFAN_DVT_config.json ${default_fsc_config_path}
    fi
  else
    echo "Run default FSC for M.2 devices"
    logger -p user.info "Run default FSC for M.2 devices"
    if [ "$fan_config" == "$FAN_CONFIG_15K" ] ; then
      cp /etc/FSC_FBGPV2_DVT_config.json ${default_fsc_config_path}
    else
      cp /etc/FSC_FBGPV2_10KFAN_DVT_config.json ${default_fsc_config_path}
    fi
  fi
  if [ $spb_type == 1 ] ; then
    # for Yv2.50
    if [ "$fan_config" == "$FAN_CONFIG_15K" ] ; then
      cp /etc/aggregate-sensor-yv250-15kfan-conf.json ${default_aggregate_config_path}
      cp /etc/FSC_FBYV250_15KFAN_DVT_config.json ${default_fsc_config_path}
    else
      cp /etc/aggregate-sensor-yv250-10kfan-conf.json ${default_aggregate_config_path}
      cp /etc/FSC_FBYV250_10KFAN_DVT_config.json ${default_fsc_config_path}
    fi
  else
    if [ "$fan_config" == "$FAN_CONFIG_15K" ] ; then
      cp /etc/aggregate-sensor-gpv2-conf.json ${default_aggregate_config_path}
    else
      cp /etc/aggregate-sensor-gpv2-10kfan-conf.json ${default_aggregate_config_path}
    fi
  fi
fi

/usr/local/bin/fan-util --set 70
runsv /etc/sv/fscd > /dev/null 2>&1 &
logger -p user.info "fscd started"

# Check SLED in/out
if [ $(gpio_get FAN_LATCH_DETECT H5) = 1 ]; then
   /usr/local/bin/fscd_end.sh $FSCD_END_SLED_OUT
elif [ "$invalid_gpv2_config" == "1" ] ; then
   /usr/local/bin/fscd_end.sh $FSCD_END_INVALID_CONFIG
else
   sv start fscd
fi

echo "done."
