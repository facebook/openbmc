#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

MAX_RETRY=600         # wait 10 mins before give up

total_daemon="sensord ipmid fscd front-paneld gpiod gpiointrd ncsid healthd ipmbd_2 ipmbd_7 ipmbd_10"

sensord_list="flag_sensord_monitor flag_sensord_health"
ipmid_list="flag_ipmid"
front_paneld_list="flag_front_led_sync flag_front_sys_status_led flag_front_health flag_front_err_code flag_front_hb"
gpiod_list="flag_gpiod_fru_miss flag_gpiod_server_pwr flag_gpiod_scc_pwr"
gpiointrd_list="flag_gpiointrd"
ncsid_list="flag_ncsid"
healthd_list="flag_healthd_wtd flag_healthd_hb_led flag_healthd_crit_proc flag_healthd_cpu flag_healthd_mem flag_healthd_ecc flag_healthd_bmc_timestamp flag_healthd_bic_fru1_health"
ipmbd_2_list="flag_ipmbd_rx_2 flag_ipmbd_res_2 flag_ipmbd_req_2"
ipmbd_7_list="flag_ipmbd_rx_7 flag_ipmbd_res_7 flag_ipmbd_req_7"
ipmbd_10_list="flag_ipmbd_rx_10 flag_ipmbd_res_10 flag_ipmbd_req_10"

check_daemon_flag() {
  flag_list=$1
  
  for flag_name in $flag_list
  do
    result="$($KV_CMD get "$flag_name")"
    if [ "$result" != "1" ] || [ "$result" = "" ]; then
      echo "$flag_name"
      break
    fi
  done
}

check_daemon_status() {
  daemon_name=$1
  
  daemon_status="$(sv status "$daemon_name" | grep 'run')"
  if [ "$daemon_status" = "" ]; then
    echo "$daemon_name"
  else
    # check ready flag of each thread
    if [ "$daemon_name" = "sensord" ]; then
      not_ready="$(check_daemon_flag "$sensord_list" "$daemon_name")"
    elif [ "$daemon" = "ipmid" ]; then
      not_ready="$(check_daemon_flag "$ipmid_list" "$daemon_name")"
    elif [ "$daemon" = "front-paneld" ]; then
      not_ready="$(check_daemon_flag "$front_paneld_list" "$daemon_name")"
    elif [ "$daemon" = "gpiod" ]; then
      not_ready="$(check_daemon_flag "$gpiod_list" "$daemon_name")"
    elif [ "$daemon" = "gpiointrd" ]; then
      not_ready="$(check_daemon_flag "$gpiointrd_list" "$daemon_name")"
    elif [ "$daemon" = "ncsid" ]; then
      not_ready="$(check_daemon_flag "$ncsid_list" "$daemon_name")"
    elif [ "$daemon" = "healthd" ]; then
      not_ready="$(check_daemon_flag "$healthd_list" "$daemon_name")"
    elif [ "$daemon" = "ipmbd_2" ]; then
      not_ready="$(check_daemon_flag "$ipmbd_2_list" "$daemon_name")"
    elif [ "$daemon" = "ipmbd_7" ]; then
      not_ready="$(check_daemon_flag "$ipmbd_7_list" "$daemon_name")"
    elif [ "$daemon" = "ipmbd_10" ]; then
      not_ready="$(check_daemon_flag "$ipmbd_10_list" "$daemon_name")"
    fi
    echo "$not_ready"
  fi
}

check_bmc_ready() {
  retry=0
    
  while [ "$retry" -lt "$MAX_RETRY" ]
  do
    NOT_READY_DAEMON=""

    for daemon in $total_daemon
    do
      result="$(check_daemon_status "$daemon")"
      
      # daemon not ready
      if [ "$result" = "$daemon" ]; then
        NOT_READY_DAEMON=$daemon
        break
      elif [ "$result" != "" ]; then
        NOT_READY_DAEMON=$daemon
        NOT_READY_FLAG=$result
        break
      fi
    done
    
    # all daemons are ready
    if [ "$NOT_READY_DAEMON" = "" ]; then
      break
    else
      sleep 1
      retry=$((retry+1))
    fi
  done
  
  if [ "$NOT_READY_DAEMON" = "" ]; then
    "$KV_CMD" set "bmc_ready_flag" "$STR_VALUE_1"
    logger -s -p user.info -t ready-flag "BMC is ready"
  else
    logger -s -p user.info -t ready-flag "daemon: $NOT_READY_DAEMON ($NOT_READY_FLAG) is not ready"
  fi
}

echo "check bmc ready..."
check_bmc_ready &
