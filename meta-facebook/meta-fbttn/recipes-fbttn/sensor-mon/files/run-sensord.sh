#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

SERVER_SDR_DONE_FILE="/tmp/sdr_server.done"
timeout_of_waiting_sdr=180  # Maximum wait 3 minutes then go ahead, because get SDR timeout is also 3 minutes.

echo -n "Setup sensor monitoring for fbttn... "

# Check for the slots present and run sensord for those slots only.
SLOTS=
  if [ $(is_server_prsnt 1) == "1" ] && [ $(is_bic_ready 1) == "1" ]; then
    SLOTS="server"
  fi

SLOTS="iom nic $SLOTS"

wait_time=0
logger -p user.info -t sensord "waiting for server SDR being ready..."
until [ $wait_time -eq $timeout_of_waiting_sdr ]
do
  if [ -f $SERVER_SDR_DONE_FILE ]; then
    logger -p user.info -t sensord "server SDR is ready."
    rm -rf $SERVER_SDR_DONE_FILE
    break
  else
    sleep 1
    wait_time=$[$wait_time+1]
  fi
done
if [ $wait_time -eq $timeout_of_waiting_sdr ]; then
  logger -p user.warn -t sensord "server SDR is not ready."
fi

exec /usr/local/bin/sensord $SLOTS

