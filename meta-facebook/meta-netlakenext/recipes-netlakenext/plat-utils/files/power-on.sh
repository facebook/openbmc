#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions
KV_STORE=/mnt/data/kv_store

#get power on reset config
power_on_reset_conf="$KV_STORE/server_power_on_reset_cfg"
last_power_state_conf="$KV_STORE/server_last_power_state"

# control the system power based on $power_on_reset_conf
power_on_reset_cfg_val=$(cat $power_on_reset_conf)
last_power_state_val=$(cat $last_power_state_conf)

if [ "$(is_bmc_power_on_reset)" -eq 1 ]; then
    if [ "$power_on_reset_cfg_val" = "on" ]; then
      /usr/local/bin/power-util server on
    elif [ "$power_on_reset_cfg_val" = "lps" ] && [ "$last_power_state_val" = "on" ]; then
      /usr/local/bin/power-util server on
    fi
fi
echo "Done"
