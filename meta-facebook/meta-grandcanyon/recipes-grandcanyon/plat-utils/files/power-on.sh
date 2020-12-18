#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

KEYDIR=/mnt/data/kv_store
IS_NEEDED_SERVER_POWER_ON=true
RETRY=0
RETRY_TIME=30
POWER_ON_TIMEOUT=0
POWER_ON_COMPLETE=1

# Check power policy
check_por_config()
{
  # Check if the file/key doesn't exist
  if [ ! -f "${KEYDIR}/server_por_cfg" ]; then
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
    logger -s -p user.info -t power-on "Power Policy: $PWR_POLICY, Last Power State: $LS"
  fi
}

# Check Server is present or not:
if [ "$(is_server_prsnt)" = "$SERVER_ABSENT" ]; then
  logger -s -p user.warn -t power-on "The Server is absent, turn off Server HSC 12V."
  $POWERUTIL_CMD server 12V-off
  
  exit 1
elif [ "$(is_server_prsnt)" = "$SERVER_PRESENT" ]; then
  logger -s -p user.info -t power-on "Server 12V-On..."
  $POWERUTIL_CMD server 12V-on

  # Wait for server finish 12V-on
  while [ $RETRY -lt $RETRY_TIME ]
  do
    FINISH="$POWER_ON_TIMEOUT"
    status="$(server_power_status)"
    if [ "$status" = "$SERVER_STATUS_DC_OFF" ]; then
      FINISH="$POWER_ON_COMPLETE"
      logger -s -p user.info -t power-on "Server 12V-On done..."
      break
    elif [ "$status" = "$SERVER_STATUS_DC_ON" ]; then
      logger -s -p user.info -t power-on "Server DC on already..."
      exit 0
    fi

    RETRY=$((RETRY+1))
    sleep 1
  done

  if [ "$FINISH" = "$POWER_ON_TIMEOUT" ]; then
    logger -s -p user.warn -t power-on "Server 12V-On timeout..."
    exit 1
  fi
  
  check_por_config
  if [ "$(server_power_status)" = "$SERVER_STATUS_DC_OFF" ] && [ "$IS_NEEDED_SERVER_POWER_ON" = true ] && [ "$FINISH" = "$POWER_ON_COMPLETE" ]; then
    logger -s -p user.info -t power-on "Server DC On..."
    $POWERUTIL_CMD server on

    # Wait for server finish DC-on
    FINISH="$POWER_ON_TIMEOUT"
    RETRY=0

    while [ $RETRY -lt $RETRY_TIME ]
    do
      FINISH=$POWER_ON_TIMEOUT
      status="$(server_power_status)"
      if [ "$status" = "$SERVER_STATUS_DC_ON" ]; then
        FINISH=$POWER_ON_COMPLETE
        logger -s -p user.info -t power-on "Server DC-On done..."
        exit 0
      fi

      RETRY=$((RETRY+1))
      sleep 1
    done

    if [ "$FINISH" = "$POWER_ON_TIMEOUT" ]; then
      logger -s -p user.warn -t power-on "Server DC-On timeout..."
      exit 1
    fi
  fi
fi

