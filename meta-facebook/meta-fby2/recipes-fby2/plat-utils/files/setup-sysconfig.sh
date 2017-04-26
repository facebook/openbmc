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

# create devices on I2C bus 1 and bus 5
# Bus1,  Glacier Point
if [[ $(is_server_prsnt 1) == "1" && $(get_slot_type 1) != "0" ]]; then

   devmem 0x1e78a084 w 0xFFF77304

   # EEPROM, 0xA2
   echo 24c128 0x51 > /sys/class/i2c-dev/i2c-1/device/new_device
   # Inlet temp sensor, 0x9A
   echo tmp75 0x4d > /sys/class/i2c-dev/i2c-1/device/new_device
   # outlet temp sensor, 0x9C
   echo tmp75 0x4e > /sys/class/i2c-dev/i2c-1/device/new_device
   # I2C mux, 0xE2
   echo pca9551 0x71 > /sys/class/i2c-dev/i2c-1/device/new_device
   # I2C GPIO, 0x40
   echo pca9551 0x20 > /sys/class/i2c-dev/i2c-1/device/new_device
   # Voltage sensor, 0x80
   echo ina230 0x40 > /sys/class/i2c-dev/i2c-1/device/new_device

fi

# Bus5,  Glacier Point
if [[ $(is_server_prsnt 3) == "1" && $(get_slot_type 3) != "0" ]]; then

   devmem 0x1e78a184 w 0xFFF77304

   # EEPROM, 0xA2
   echo 24c128 0x51 > /sys/class/i2c-dev/i2c-5/device/new_device
   # Inlet temp sensor, 0x9A
   echo tmp75 0x4d > /sys/class/i2c-dev/i2c-5/device/new_device
   # outlet temp sensor, 0x9C
   echo tmp75 0x4e > /sys/class/i2c-dev/i2c-5/device/new_device
   # I2C mux, 0xE2
   echo pca9551 0x71 > /sys/class/i2c-dev/i2c-5/device/new_device
   # I2C GPIO, 0x40
   echo pca9551 0x20 > /sys/class/i2c-dev/i2c-5/device/new_device
   # Voltage sensor, 0x80
   echo ina230 0x40 > /sys/class/i2c-dev/i2c-5/device/new_device

fi

echo "done"
