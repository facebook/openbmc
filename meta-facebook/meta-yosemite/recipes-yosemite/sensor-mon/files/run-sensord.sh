#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup sensor monitoring for yosemite... "

# Check for the slots present and run sensord for those slots only.
SLOTS=
  if [ $(is_server_prsnt 1) == "1" ] && [ $(is_bic_ready 1) == "1" ]; then
    SLOTS="$SLOTS slot1"
  fi

  if [ $(is_server_prsnt 2) == "1" ] && [ $(is_bic_ready 2) == "1" ]; then
    SLOTS="$SLOTS slot2"
  fi

  if [ $(is_server_prsnt 3) == "1" ] && [ $(is_bic_ready 3) == "1" ]; then
    SLOTS="$SLOTS slot3"
  fi

  if [ $(is_server_prsnt 4) == "1" ] && [ $(is_bic_ready 4) == "1" ]; then
    SLOTS="$SLOTS slot4"
  fi

SLOTS="$SLOTS spb nic"
exec /usr/local/bin/sensord $SLOTS
