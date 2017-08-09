#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup sensor monitoring for fbttn... "

# Check for the slots present and run sensord for those slots only.
SLOTS=
  if [ $(is_server_prsnt 1) == "1" ] && [ $(is_bic_ready 1) == "1" ]; then
    SLOTS="server"
  fi

SLOTS="iom nic $SLOTS"
exec /usr/local/bin/sensord $SLOTS

