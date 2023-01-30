#!/bin/bash

# shellcheck source=meta-facebook/meta-fby35/recipes-fby35/plat-utils/files/ast-functions
. /usr/local/fbpackages/utils/ast-functions

SLOTS=

init_class2_sensord() {
  SLOTS=(slot1 bmc nic)
}

init_class1_sensord() {
  sys_config="$($KV_CMD get sled_system_conf persistent)"
  if [[ "$sys_config" == "Type_8" ]]; then
    SLOTS=(slot1)
  elif [[ "$sys_config" =~ ^(Type_(DPV2|HD))$ ]]; then
    SLOTS=(slot1 slot3)
  else
    SLOTS=(slot1 slot2 slot3 slot4)
  fi

  SLOTS+=(bmc nic)
}

bmc_location=$(get_bmc_board_id)
if [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
  #The BMC of class2
  init_class2_sensord
elif [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
  #The BMC of class1
  init_class1_sensord
else
  /bin/echo -n "Is board id correct(id=$bmc_location)?..."
fi

exec /usr/local/bin/sensord "${SLOTS[@]}"
