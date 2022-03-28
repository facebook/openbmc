#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions
echo -n "Setup sensor monitoring for FBY3... "
SLOTS=

function get_bus_num() {
  local bus=0
  case $1 in
    slot1)
      bus=4
    ;;
    slot2)
      bus=5
    ;;
    slot3)
      bus=6
    ;;
    slot4)
      bus=7
    ;;
  esac

  echo $bus
}

read_system_conf() {
  slot_num=${1#"slot"}
  val=$(get_m2_prsnt_sts "$1")
  i2c_bus=$(get_bus_num $1)

  system_type="Type_"
  #0 = 2ou and 1ou are present
  #4 = 2ou is present
  #8 = 1ou is present
  #other = not present
  if [ "$val" -eq 0 ]; then
    exp_board=$(get_2ou_board_type $i2c_bus)

    if [ "$exp_board" == "0x04" ] || [ "$exp_board" == "0x00" ] || [ "$exp_board" == "0x03" ]; then
      system_type=${system_type}17
    else
      system_type=${system_type}10
    fi      
  elif [ "$val" -eq 4 ]; then
    cpld_bus=$(get_cpld_bus "$slot_num")
    type_2ou=$(get_2ou_board_type "$cpld_bus")
    if [ "$type_2ou" = "0x06" ]; then
      # DP slot
      dp_type=$(/usr/bin/kv get "sled_system_conf" persistent)
      if [ "$dp_type" = "Type_DPB" ]; then
        system_type=${system_type}DPB
      else
        system_type=${system_type}DP
      fi
    elif [ "$type_2ou" == "0x03" ] ; then # Config D GPv3
        system_type=${system_type}15
    else
      system_type=${system_type}1
    fi
  elif [ "$val" -eq 8 ]; then
    system_type=${system_type}10
  else
    system_type=${system_type}1
  fi

  /usr/bin/kv set "${1}_system_conf" "$system_type" persistent
}

init_class2_sensord() {
  SLOTS="slot1 bmc nic"
  read_system_conf "slot1"

  board=$(get_2ou_board_type 4)
  if [ "$board" = "0x04" ]; then
    SLOTS="$SLOTS slot1-2U-exp slot1-2U-top slot1-2U-bot"
  fi
}

init_class1_sensord() {
  # Check for the slots present and run sensord for those slots only.
  if [ "$(is_server_prsnt 1)" = "1" ]; then
    if [ "$(is_slot_12v_on 1)" = "1" ]; then
      SLOTS="$SLOTS slot1"
      read_system_conf "slot1"
    fi
  fi

  if [ "$(is_server_prsnt 2)" = "1" ]; then
    if [ "$(is_slot_12v_on 2)" = "1" ]; then
      SLOTS="$SLOTS slot2"
      read_system_conf "slot2"
    fi
  fi

  if [ "$(is_server_prsnt 3)" = "1" ]; then
    if [ "$(is_slot_12v_on 3)" = "1" ]; then
      SLOTS="$SLOTS slot3"
      read_system_conf "slot3"
    fi
  fi

  if [ "$(is_server_prsnt 4)" = "1" ]; then
    if [ "$(is_slot_12v_on 4)" = "1" ]; then
      SLOTS="$SLOTS slot4"
      read_system_conf "slot4"
    fi
  fi

  SLOTS="$SLOTS bmc nic"
}

bmc_location=$(get_bmc_board_id)
if [ "$bmc_location" -eq 9 ]; then
  #The BMC of class2
  init_class2_sensord
elif [ "$bmc_location" -eq 14 ] || [ "$bmc_location" -eq 7 ]; then
  #The BMC of class1
  init_class1_sensord
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

exec /usr/local/bin/sensord $SLOTS
