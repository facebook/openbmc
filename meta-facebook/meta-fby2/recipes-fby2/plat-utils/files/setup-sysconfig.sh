# This file contains definitions for the different system configurations. 
# We should probably move some more of the definitions to this file at some point.

echo -n "Setup System Configuration .."

. /usr/local/fbpackages/utils/ast-functions

# Enable buffer to pass through signal by system configuration
if [ $(is_server_prsnt 2) == "1" ] && [ $(get_slot_type 2) == "0" ] ; then

  if [ $(is_server_prsnt 1) == "1" ] && [ $(get_slot_type 1) != "0" ] ; then
     gpio_set B4 0
     gpio_set B5 0
  fi
fi				  
  
if [ $(is_server_prsnt 4) == "1" ] && [ $(get_slot_type 4) == "0" ] ; then

  if [ $(is_server_prsnt 3) == "1" ] && [ $(get_slot_type 3) != "0" ] ; then
     gpio_set B6 0
     gpio_set B7 0
  fi
fi

echo "done"
