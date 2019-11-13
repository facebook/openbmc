#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup sensor monitoring for fby3-poc... "
SLOTS=

function init_class2_sensor() {
  SLOTS="spb"
}

function init_class1_ipmb() {
  # Check for the slots present and run sensord for those slots only.
  if [ $(is_server_prsnt 0) == "1" ]; then
    if [ $(is_slot_12v_on 0) == "1" ]; then
      SLOTS="$SLOTS slot1"
    fi
  fi

  if [ $(is_server_prsnt 1) == "1" ]; then
    if [ $(is_slot_12v_on 1) == "1" ]; then
      SLOTS="$SLOTS slot2"
    fi
  fi

  if [ $(is_server_prsnt 2) == "1" ]; then
    if [ $(is_slot_12v_on 2) == "1" ]; then
      SLOTS="$SLOTS slot3"
    fi
  fi

  if [ $(is_server_prsnt 3) == "1" ]; then
    if [ $(is_slot_12v_on 3) == "1" ]; then
      SLOTS="$SLOTS slot4"
    fi
  fi

  SLOTS="$SLOTS spb nic"
}

bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  #The BMC of class2
  init_class2_ipmb
elif [ $bmc_location -eq 14 ]; then
  #The BMC of class1
  init_class1_ipmb
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

exec /usr/local/bin/sensord $SLOTS

if [ ! -n "$SLOTS" ]; then
  echo "sensord is stopped"
  sv stop sensord
fi
