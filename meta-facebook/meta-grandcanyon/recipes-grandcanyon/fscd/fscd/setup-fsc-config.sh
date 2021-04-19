#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions

retry=$MAX_RETRY

while [ $retry -gt 0 ]
do
  uic_location=$("$EXPANDERUTIL_CMD" "$NETFN_EXPANDER_REQ" "$CMD_GET_UIC_LOCATION")
  ret=$?
  
  if [ "$ret" = "0" ]; then
    uic_location=$((10#${uic_location}))
    break
  fi
  
  ((retry--))
  sleep 1
done

set_default_fan_table="/usr/local/bin/set_default_fan_table.py"

default_fsc_config_path="/etc/fsc-config.json"
fsc_type5_default_config_path="/etc/FSC_GC_EVT_default_config.json"
fsc_type5_config_path="/etc/FSC_GC_Type5_EVT_v1_config.json"
fsc_type7_config_path="/etc/FSC_GC_Type7_EVT_v1_config.json"

fsc_type5_zone_config_path="/etc/fsc/FSC_GC_Type5_EVT_v1_zone0.fsc"

get_chassis_type
chassis_type=$?

if [ "$chassis_type" = "$UIC_TYPE_7_HEADNODE" ]; then
  cp ${fsc_type7_config_path} ${default_fsc_config_path}
  echo "Setup fscd: Use Type 7 fan table... "
elif [ "$chassis_type" = "$UIC_TYPE_5" ]; then
  cp ${fsc_type5_config_path} ${default_fsc_config_path}
  if [ "$uic_location" = "$UIC_LOCATION_A" ]; then
    sed -i "s/_x/_a/g" ${fsc_type5_zone_config_path}
    echo "Setup fscd: Use Type 5 side A fan table... "
  elif [ "$uic_location" = "$UIC_LOCATION_B" ]; then
    sed -i "s/_x/_b/g" ${fsc_type5_zone_config_path}
    echo "Setup fscd: Use Type 5 side B fan table... "
  else
    echo "Setup fscd: UIC location is unknown. Please confirm the UIC location... "
  fi
else
  python ${set_default_fan_table}
  cp ${fsc_type5_default_config_path} ${default_fsc_config_path}
  echo "Setup fscd: Unknown chassis type. Use default fan table. Please confirm chassis type... "
fi

runsv /etc/sv/fscd > /dev/null 2>&1 &

echo "Setup fan speed done... "
