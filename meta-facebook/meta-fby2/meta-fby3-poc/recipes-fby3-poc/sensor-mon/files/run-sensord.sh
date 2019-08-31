#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup sensor monitoring for fby3-poc... "
SLOTS=

function expansion_init() {
  SLOTS="spb"
}

function server_init() {
  # Check for the slots present and run sensord for those slots only.
  if [ $(is_server_prsnt 1) == "1" ]; then
    if [ $(is_slot_12v_on 1) == "1" ]; then
      SLOTS="$SLOTS slot1"
    fi
  fi

  if [ $(is_server_prsnt 2) == "1" ]; then
    if [ $(is_slot_12v_on 2) == "1" ]; then
      SLOTS="$SLOTS slot2"
    fi
  fi

  if [ $(is_server_prsnt 3) == "1" ]; then
    if [ $(is_slot_12v_on 3) == "1" ]; then
      SLOTS="$SLOTS slot3"
    fi
  fi

  if [ $(is_server_prsnt 4) == "1" ]; then
    if [ $(is_slot_12v_on 4) == "1" ]; then
      SLOTS="$SLOTS slot4"
    fi
  fi

  SLOTS="$SLOTS spb nic"
}

Location=$(gpio_get BMC_LOCATION)
#Location: 1 exp; 0 baseboard
if [ $Location == "1" ]; then
  expansion_init
else
  server_init
fi

exec /usr/local/bin/sensord $SLOTS
