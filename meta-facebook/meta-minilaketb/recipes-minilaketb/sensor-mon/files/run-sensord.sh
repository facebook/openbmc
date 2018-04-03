#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup sensor monitoring for minilaketb... "

# Check for the slots present and run sensord for those slots only.
SLOTS=
  if [ $(is_server_prsnt 1) == "1" ]; then
    if [ $(is_slot_12v_on 1) == "1" ]; then
      if [[ $(get_slot_type 1) != "0" || $(is_bic_ready 1) == "1" ]]; then
        SLOTS="$SLOTS slot1"
      fi
    else
      SLOTS="$SLOTS slot1"
    fi
  fi


SLOTS="$SLOTS spb"
exec /usr/local/bin/sensord $SLOTS
