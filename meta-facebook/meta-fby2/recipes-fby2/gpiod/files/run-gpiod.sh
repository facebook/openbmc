#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

# Check for the slots present and run sensord for those slots only.
SLOTS=
  #Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
  #get_slot_type is to get slot type to check if the slot type is server
  if [[ $(is_server_prsnt 1) == "1" && $(get_slot_type 1) == "0" ]]; then
    SLOTS="$SLOTS slot1"
  fi
  #Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
  #get_slot_type is to get slot type to check if the slot type is server
  if [[ $(is_server_prsnt 2) == "1" && $(get_slot_type 2) == "0" ]]; then
    SLOTS="$SLOTS slot2"
  fi
  #Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
  #get_slot_type is to get slot type to check if the slot type is server
  if [[ $(is_server_prsnt 3) == "1" && $(get_slot_type 3) == "0" ]]; then
    SLOTS="$SLOTS slot3"
  fi
  #Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
  #get_slot_type is to get slot type to check if the slot type is server
  if [[ $(is_server_prsnt 4) == "1" && $(get_slot_type 4) == "0" ]]; then
    SLOTS="$SLOTS slot4"
  fi

exec /usr/local/bin/gpiod $SLOTS 
