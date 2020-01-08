#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

SLOTS=

  if [[ $(is_server_prsnt 1) == "1" ]]; then
    SLOTS="$SLOTS slot1"
  fi

  if [[ $(is_server_prsnt 2) == "1" ]]; then
    SLOTS="$SLOTS slot2"
  fi

  if [[ $(is_server_prsnt 3) == "1" ]]; then
    SLOTS="$SLOTS slot3"
  fi

  if [[ $(is_server_prsnt 4) == "1" ]]; then
    SLOTS="$SLOTS slot4"
  fi

exec /usr/local/bin/gpiod $SLOTS 
