#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

MAX_RETRY=600         # wait 10 mins before give up
NOT_READY_DAEMON=""

total_daemon="sensord ipmid fscd front-paneld gpiod gpiointrd ncsid healthd ipmbd_2 ipmbd_7 ipmbd_10"

check_daemon_status() {
  daemon_name=$1
  
  daemoe_status="$(sv status "$daemon_name" | grep 'run')"
  if [ "$daemoe_status" = "" ]; then
    echo "$daemon_name"
  fi
}

check_bmc_ready() {
  retry=0
    
  while [ "$retry" -lt "$MAX_RETRY" ]
  do
  
    for daemon in $total_daemon
    do
      NOT_READY_DAEMON="$(check_daemon_status "$daemon")"
      
      # daemon not ready
      if [ "$NOT_READY_DAEMON" = "$daemon" ]; then
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
    logger -s -p user.info -t ready-flag "daemon: $NOT_READY_DAEMON is not ready"
  fi
}

echo "check bmc ready..."
check_bmc_ready &
