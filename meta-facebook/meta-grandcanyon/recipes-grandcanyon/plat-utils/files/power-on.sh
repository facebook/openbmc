#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

KEYDIR=/mnt/data/kv_store
IS_NEEDED_SERVER_POWER_ON=true
RETRY=0
RETRY_TIME=30
POWER_12V_ON_TIMEOUT=0
POWER_12V_ON_COMPLETE=1
SYNC_DATE=/usr/local/bin/sync_date.sh

# Check power policy
check_por_config()
{
  # Check if the file/key doesn't exist
  if [ ! -f "${KEYDIR}/server_por_cfg" ]; then
    PWR_POLICY="on"
    IS_NEEDED_SERVER_POWER_ON=true
  else
    PWR_POLICY=$(cat ${KEYDIR}/server_por_cfg)

    # Case ON
    if [ "$PWR_POLICY" = "on" ]; then
      IS_NEEDED_SERVER_POWER_ON=true

    # Case OFF
    elif [ "$PWR_POLICY" = "off" ]; then
      IS_NEEDED_SERVER_POWER_ON=false

    # Case LPS
    elif [ "$PWR_POLICY" = "lps" ]; then

      # Check if the file/key doesn't exist
      if [ ! -f "${KEYDIR}/pwr_server_last_state" ]; then
        LS="on"
        IS_NEEDED_SERVER_POWER_ON=true
      else
        LS=$(cat ${KEYDIR}/pwr_server_last_state)
        if [ "$LS" = "on" ]; then
          IS_NEEDED_SERVER_POWER_ON=true
        elif [ "$LS" = "off" ]; then
          IS_NEEDED_SERVER_POWER_ON=false
        fi
      fi
    fi
  fi

  if [ "$PWR_POLICY" = "lps" ]; then
    logger -s -p user.info -t power-on "Power Policy: $PWR_POLICY, Last Power State: $LS"
  else
    logger -s -p user.info -t power-on "Power Policy: $PWR_POLICY"
  fi
}

# Check Server is present or not:
if [ "$(is_server_prsnt)" = "$SERVER_ABSENT" ]; then
  logger -s -p user.warn -t power-on "The Server is absent, turn off Server HSC 12V."
  $POWERUTIL_CMD server 12V-off
  $SYNC_DATE
  exit 1
elif [ "$(is_server_prsnt)" = "$SERVER_PRESENT" ]; then
  if [ "$(is_bmc_por)" = "$STR_VALUE_0" ]; then  # BMC is not Power-On-Reset (POR, AC-on)
    logger -s -p user.info -t power-on "BMC is not POR"
    $SYNC_DATE
    exit 0
  fi

  logger -s -p user.info -t power-on "Server 12V-On..."
  $POWERUTIL_CMD server 12V-on

  RETRY=0
  FINISH="$POWER_12V_ON_TIMEOUT"
  # Wait for server finish 12V-on
  while [ $RETRY -lt $RETRY_TIME ]
  do
    if [ "$(is_server_12v_on)" = true ]; then
      FINISH="$POWER_12V_ON_COMPLETE"
      logger -s -p user.info -t power-on "Server 12V-On done"
      break
    fi

    RETRY=$((RETRY+1))
    sleep 1
  done

  if [ "$FINISH" = "$POWER_12V_ON_TIMEOUT" ]; then
    logger -s -p user.warn -t power-on "Server 12V-On timeout"
    $SYNC_DATE
    exit 1
  fi

  check_por_config
  # Check server DC status according to power policy
  if [ "$IS_NEEDED_SERVER_POWER_ON" = true ]; then
    RETRY=0

    while [ $RETRY -lt $RETRY_TIME ]
    do
      status="$(server_power_status)"
      if [ "$status" = "$SERVER_STATUS_DC_ON" ]; then
        logger -s -p user.info -t power-on "Server DC-On done"
        $SYNC_DATE
        exit 0
      fi

      RETRY=$((RETRY+1))
      sleep 1
    done

    logger -s -p user.warn -t power-on "Server DC-On timeout"
    $SYNC_DATE
    exit 1
  fi
  $SYNC_DATE
fi

exit 0
