#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
### BEGIN INIT INFO
# Provides:          setup-bic-cached
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set Cachcing for Bridge IC info
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup Caching for Bridge IC info.."
#Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
#get_slot_type is to get slot type to check if the slot type is server
if [[ $(is_server_prsnt 1) == "1" ]] && [[ $(get_slot_type 1) == "0" || $(get_slot_type 1) == "4" ]]; then
  /usr/local/bin/bic-cached 1 > /dev/null 2>&1 &
fi
#Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
#get_slot_type is to get slot type to check if the slot type is server
if [[ $(is_server_prsnt 2) == "1" ]] && [[ $(get_slot_type 2) == "0" || $(get_slot_type 2) == "4" ]]; then
  /usr/local/bin/bic-cached 2 > /dev/null 2>&1 &
fi
#Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
#get_slot_type is to get slot type to check if the slot type is server
if [[ $(is_server_prsnt 3) == "1" ]] && [[ $(get_slot_type 3) == "0" || $(get_slot_type 3) == "4" ]]; then
  /usr/local/bin/bic-cached 3 > /dev/null 2>&1 &
fi
#Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
#get_slot_type is to get slot type to check if the slot type is server
if [[ $(is_server_prsnt 4) == "1" ]] && [[ $(get_slot_type 4) == "0" || $(get_slot_type 4) == "4" ]]; then
  /usr/local/bin/bic-cached 4 > /dev/null 2>&1 &
fi

echo "done."
