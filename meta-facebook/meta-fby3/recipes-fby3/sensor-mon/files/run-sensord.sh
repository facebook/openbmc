#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions
echo -n "Setup sensor monitoring for FBY3... "
SLOTS=

function init_class2_sensord() {
  SLOTS="slot1 bmc nic"
}

function init_class1_sensord() {
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

  SLOTS="$SLOTS bmc nic"
}

bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  #The BMC of class2
  init_class2_sensord
elif [ $bmc_location -eq 14 ]; then
  #The BMC of class1
  init_class1_sensord
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

exec /usr/local/bin/sensord $SLOTS
