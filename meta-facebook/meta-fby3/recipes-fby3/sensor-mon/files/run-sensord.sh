#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions
echo -n "Setup sensor monitoring for FBY3... "
SLOTS=

function read_system_conf() {
  val=$(get_m2_prsnt_sts $1)

  system_type="Type_"
  #0 = 2ou and 1ou are present
  #4 = 2ou is present
  #8 = 1ou is present
  #other = not present
  if [ $val = 0 ]; then
    system_type=${system_type}10
  elif [ $val = 4 ]; then
    system_type=${system_type}1
  elif [ $val = 8 ]; then
    system_type=${system_type}10
  else
    system_type=${system_type}1
  fi

  echo -n "$system_type" > /mnt/data/kv_store/${1}_system_conf
}

function init_class2_sensord() {
  SLOTS="slot1 bmc nic"
  read_system_conf "slot1"
}

function init_class1_sensord() {
  # Check for the slots present and run sensord for those slots only.
  if [ $(is_server_prsnt 1) == "1" ]; then
    if [ $(is_slot_12v_on 1) == "1" ]; then
      SLOTS="$SLOTS slot1"
      read_system_conf "slot1"
    fi
  fi

  if [ $(is_server_prsnt 2) == "1" ]; then
    if [ $(is_slot_12v_on 2) == "1" ]; then
      SLOTS="$SLOTS slot2"
      read_system_conf "slot2"
    fi
  fi

  if [ $(is_server_prsnt 3) == "1" ]; then
    if [ $(is_slot_12v_on 3) == "1" ]; then
      SLOTS="$SLOTS slot3"
      read_system_conf "slot3"
    fi
  fi

  if [ $(is_server_prsnt 4) == "1" ]; then
    if [ $(is_slot_12v_on 4) == "1" ]; then
      SLOTS="$SLOTS slot4"
      read_system_conf "slot4"
    fi
  fi

  SLOTS="$SLOTS bmc nic"
}

bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  #The BMC of class2
  init_class2_sensord
elif [ $bmc_location -eq 14 ] || [ $bmc_location -eq 7 ]; then
  #The BMC of class1
  init_class1_sensord
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

exec /usr/local/bin/sensord $SLOTS
