#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions

FAN_SOURCE="DELTA"

wait_sensord_ready()
{
  RETRY=180   #timeout: 3 minutes
  while [ $RETRY -gt 0 ]
  do
    ret=$($KV_CMD get flag_sensord_monitor)
    if [ "$ret" = "1" ]; then
      return 0
    fi
    RETRY=$((RETRY-1))
    sleep 1
  done

  logger -s -p user.warn -t setup-fsc-config "Waiting sensord ready timeout"
  return 1
}

wait_fan_fru_dumped()
{
  RETRY=180   #timeout: 3 minutes
  i=0

  #Check fan present
  while [ $i -lt "$FAN_FRU_CNT" ]
  do
    if [ "$(gpio_get FAN_${i}_INS_N)" != "$STR_VALUE_0" ]; then
      echo $i
      return
    fi
    i=$((i+1))
  done

  #Check fan FRU dumped
  i=0
  for _ in $(seq $RETRY);  do
    while [ $i -lt "$FAN_FRU_CNT" ]
    do
      ret=$($KV_CMD get fan${i}_dumped)
      [ "$ret" != "$STR_VALUE_1" ] && break
      i=$((i+1))
    done
    [ $i -eq "$FAN_FRU_CNT" ] && break
    sleep 1
  done

  echo $i
}

check_fan_source()
{
  i=0
  AVC_CNT=0

  while [ $i -lt "$FAN_FRU_CNT" ]
  do
    fan_source=$($FRUIDUTIL_CMD fan${i} 2>/dev/null | grep 'Product Manufacturer')
    ret=$?
    # Get fan FRU failed
    if [ "$ret" -ne "0" ]; then
      logger -s -p user.warn -t setup-fsc-config "Get fan${i} FRU failed, use DELTA fan table."
      FAN_SOURCE="DELTA"
      return
    fi

    fan_source=${fan_source##*: }

    if [ "$fan_source" = "AVC" ]; then
      AVC_CNT=$((AVC_CNT+1))
    fi
    i=$((i+1))
  done

  if [ $AVC_CNT -eq "$FAN_FRU_CNT" ]; then
    FAN_SOURCE="AVC"
  elif [ $AVC_CNT -gt 0 ]; then
    logger -s -p user.warn -t setup-fsc-config "Mix using fan source, use DELTA fan table."
    FAN_SOURCE="DELTA"
  fi
}

set_default_fan_table="/usr/local/bin/set_default_fan_table.py"

default_fsc_config_path="/etc/fsc-config.json"
fsc_type5_default_config_path="/etc/FSC_GC_PVT_default_config.json"
fsc_type5_avc_config_path="/etc/FSC_GC_Type5_PVT_AVC_v1_config.json"
fsc_type5_delta_config_path="/etc/FSC_GC_Type5_PVT_DELTA_v1_config.json"
fsc_type7_avc_config_path="/etc/FSC_GC_Type7_PVT_AVC_v1_config.json"
fsc_type7_delta_config_path="/etc/FSC_GC_Type7_PVT_DELTA_v1_config.json"

fsc_type5_zone_config_path="/etc/fsc/FSC_GC_Type5_PVT_v1_zone0.fsc"

get_uic_location
uic_location=$?

get_chassis_type
chassis_type=$?

# Check fan source to use different fan table
fan_fru_dumped_cnt=$(wait_fan_fru_dumped)
if [ "$fan_fru_dumped_cnt" -eq "$FAN_FRU_CNT" ]; then
  check_fan_source
else
  logger -s -p user.warn -t setup-fsc-config "Waiting fan${fan_fru_dumped_cnt} dumped timeout or fan${fan_fru_dumped_cnt} not present, use DELTA fan table."
fi

if [ "$chassis_type" = "$UIC_TYPE_7_HEADNODE" ]; then
  if [ "$FAN_SOURCE" = "AVC" ]; then
    cp ${fsc_type7_avc_config_path} ${default_fsc_config_path}
  else
    cp ${fsc_type7_delta_config_path} ${default_fsc_config_path}
  fi
  echo "Setup fscd: Use Type 7 ${FAN_SOURCE} fan table... "
elif [ "$chassis_type" = "$UIC_TYPE_5" ]; then
  if [ "$FAN_SOURCE" = "AVC" ]; then
    cp ${fsc_type5_avc_config_path} ${default_fsc_config_path}
  else
    cp ${fsc_type5_delta_config_path} ${default_fsc_config_path}
  fi

  if [ "$uic_location" = "$UIC_LOCATION_A" ]; then
    sed -i "s/_x/_a/g" ${fsc_type5_zone_config_path}
    echo "Setup fscd: Use Type 5 side A ${FAN_SOURCE} fan table... "
  elif [ "$uic_location" = "$UIC_LOCATION_B" ]; then
    sed -i "s/_x/_b/g" ${fsc_type5_zone_config_path}
    echo "Setup fscd: Use Type 5 side B ${FAN_SOURCE} fan table... "
  else
    echo "Setup fscd: UIC location is unknown. Please confirm the UIC location... "
  fi
else
  python ${set_default_fan_table}
  cp ${fsc_type5_default_config_path} ${default_fsc_config_path}
  echo "Setup fscd: Unknown chassis type. Use default fan table. Please confirm chassis type... "
fi

wait_sensord_ready

runsv /etc/sv/fscd > /dev/null 2>&1 &

echo "Setup fan speed done... "
