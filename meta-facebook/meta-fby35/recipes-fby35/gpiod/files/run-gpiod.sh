#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

SLOTS=

init_class2_gpiod() {
  SLOTS="$SLOTS slot1"
}

init_class1_gpiod() {
  if [ "$(is_server_prsnt 1)" = "1" ]; then
    SLOTS="$SLOTS slot1"
  fi

  if [ "$(is_server_prsnt 2)" = "1" ]; then
    SLOTS="$SLOTS slot2"
  fi

  if [ "$(is_server_prsnt 3)" = "1" ]; then
    SLOTS="$SLOTS slot3"
  fi

  if [ "$(is_server_prsnt 4)" = "1" ]; then
    SLOTS="$SLOTS slot4"
  fi
}

bmc_location=$(get_bmc_board_id)
if [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
  #The BMC of class2
  init_class2_gpiod
elif [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
  #The BMC of class1
  init_class1_gpiod
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

exec /usr/local/bin/gpiod $SLOTS
