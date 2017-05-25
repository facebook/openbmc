#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup sensor monitoring for fbttn... "

# Check for the slots present and run sensord for those slots only.
SLOTS=
  if [ $(is_server_prsnt 1) == "1" ] && [ $(is_bic_ready 1) == "1" ]; then
    SLOTS="$SLOTS slot1"
  fi

SLOTS="$SLOTS iom nic"
exec /usr/local/bin/sensord $SLOTS

