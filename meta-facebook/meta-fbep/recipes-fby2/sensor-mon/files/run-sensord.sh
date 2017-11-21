#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup sensor monitoring for fby2... "

# Check for the slots present and run sensord for those slots only.
SLOTS=
  if [ $(is_server_prsnt 1) == "1" ] && [[ $(get_slot_type 1) != "0" || $(is_bic_ready 1) == "1" ]]; then
    SLOTS="$SLOTS slot1"
  fi

  if [ $(is_server_prsnt 2) == "1" ] && [[ $(get_slot_type 2) != "0" || $(is_bic_ready 2) == "1" ]]; then
    SLOTS="$SLOTS slot2"
  fi

  if [ $(is_server_prsnt 3) == "1" ] && [[ $(get_slot_type 3) != "0" || $(is_bic_ready 3) == "1" ]]; then
    SLOTS="$SLOTS slot3"
  fi

  if [ $(is_server_prsnt 4) == "1" ] && [[ $(get_slot_type 4) != "0" || $(is_bic_ready 4) == "1" ]]; then
    SLOTS="$SLOTS slot4"
  fi

SLOTS="$SLOTS spb nic"
exec /usr/local/bin/sensord $SLOTS
