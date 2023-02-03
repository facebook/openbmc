#!/bin/sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# shellcheck source=meta-facebook/meta-fby35/recipes-fby35/plat-utils/files/ast-functions
. /usr/local/fbpackages/utils/ast-functions

MAX_RETRY=600         # wait 10 mins before give up
NOT_READY_DAEMON=""

total_daemon="sensord ipmid fscd front-paneld gpiod gpiointrd ncsid healthd ipmbd_9"

sensord_list="flag_sensord_monitor flag_sensord_health"
ipmid_list="flag_ipmid"
front_paneld_list="flag_front_sys_status_led"
gpiod_list="flag_gpiod_server_pwr "
gpiointrd_list="flag_gpiointrd flag_gpiod_fru_miss"
ncsid_list="flag_ncsid"
healthd_list="flag_healthd_wtd flag_healthd_hb_led flag_healthd_crit_proc flag_healthd_cpu flag_healthd_mem flag_healthd_bmc_timestamp"
ipmbd_9_list="flag_ipmbd_rx_9 flag_ipmbd_res_9 flag_ipmbd_req_9"

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

  #check slot present & BIC ready
  if [ "$daemon_name" = "mctpd_1" ]; then
    if [ "$(is_sb_bic_ready 1)" != "1" ]; then
      return
    fi
  elif [ "$daemon_name" = "mctpd_2" ]; then
    if [ "$(is_sb_bic_ready 2)" != "1" ]; then
      return
    fi
  elif [ "$daemon_name" = "mctpd_3" ]; then
    if [ "$(is_sb_bic_ready 3)" != "1" ]; then
      return
    fi
  elif [ "$daemon_name" = "mctpd_4" ]; then
    if [ "$(is_sb_bic_ready 4)" != "1" ]; then
      return
    fi
  fi

  daemon_status="$(sv status "$daemon_name" | grep 'run')"
  if [ "$daemon_status" = "" ]; then
    echo "$daemon_name"
  else
    # check ready flag of each thread
    if [ "$daemon_name" = "sensord" ]; then
      not_ready="$(check_daemon_flag "$sensord_list" "$daemon_name")"
    elif [ "$daemon_name" = "ipmid" ]; then
      not_ready="$(check_daemon_flag "$ipmid_list" "$daemon_name")"
    elif [ "$daemon_name" = "front-paneld" ]; then
      not_ready="$(check_daemon_flag "$front_paneld_list" "$daemon_name")"
    elif [ "$daemon_name" = "gpiod" ]; then
      not_ready="$(check_daemon_flag "$gpiod_list" "$daemon_name")"
    elif [ "$daemon_name" = "gpiointrd" ]; then
      not_ready="$(check_daemon_flag "$gpiointrd_list" "$daemon_name")"
    elif [ "$daemon_name" = "ncsid" ]; then
      not_ready="$(check_daemon_flag "$ncsid_list" "$daemon_name")"
    elif [ "$daemon_name" = "healthd" ]; then
      not_ready="$(check_daemon_flag "$healthd_list" "$daemon_name")"
    elif [ "$daemon_name" = "ipmbd_9" ]; then
      not_ready="$(check_daemon_flag "$ipmbd_9_list" "$daemon_name")"
    else
      return
    fi
    echo "$not_ready"
  fi
}

check_bmc_ready() {
  retry=0

  while [ "$retry" -lt "$MAX_RETRY" ]
  do

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
      else
        NOT_READY_DAEMON=
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
    "$KV_CMD" set bmc_ready_flag 1
    logger -s -p user.info -t ready-flag "BMC is ready"
  else
    logger -s -p user.info -t ready-flag "daemon: $NOT_READY_DAEMON ($NOT_READY_FLAG) is not ready"
  fi
}

echo "check bmc ready..."
check_bmc_ready &
