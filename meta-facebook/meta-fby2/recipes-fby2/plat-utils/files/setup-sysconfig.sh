# This file contains definitions for the different system configurations. 
# We should probably move some more of the definitions to this file at some point.

echo -n "Setup System Configuration .."

. /usr/local/fbpackages/utils/ast-functions

SPB_REV=$(($(($(($(gpio_get BOARD_REV_ID2 Y2) * 4)) + $(($(gpio_get BOARD_REV_ID1 Y1) * 2)))) + $(gpio_get BOARD_REV_ID0 Y0)))
echo $SPB_REV > /tmp/spb_rev

if [ $(is_server_prsnt 2) == "1" ] ; then
   if [ $(get_slot_type 2) != "0" ] ; then
      gpio_set P12V_STBY_SLOT2_EN O5 0
      logger -p user.crit "Invalid configuration on SLOT2"
      if [ $(is_server_prsnt 1) == "1" ] && [ $(get_slot_type 1) != "0" ] ; then
         gpio_set P12V_STBY_SLOT1_EN O4 0
      fi
   fi
else
   if [ $(is_server_prsnt 1) == "1" ] && [ $(get_slot_type 1) != "0" ] ; then
      gpio_set P12V_STBY_SLOT1_EN O4 0
   fi
fi

if [ $(is_server_prsnt 4) == "1" ] ; then
   if [ $(get_slot_type 4) != "0" ] ; then
      gpio_set P12V_STBY_SLOT4_EN O7 0
      logger -p user.crit "Invalid configuration on SLOT4"
      if [ $(is_server_prsnt 3) == "1" ] && [ $(get_slot_type 3) != "0" ] ; then
         gpio_set P12V_STBY_SLOT3_EN O6 0
      fi
   fi
else
   if [ $(is_server_prsnt 3) == "1" ] && [ $(get_slot_type 3) != "0" ] ; then
      gpio_set P12V_STBY_SLOT3_EN O6 0
   fi
fi

# Enable buffer to pass through signal by system configuration
if [ $(is_server_prsnt 1) == "1" ] && [ $(is_server_prsnt 2) == "1" ] ; then
   if [ $(get_slot_type 1) != "0" ] && [ $(get_slot_type 2) == "0" ] ; then
      gpio_set CLK_BUFF1_PWR_EN_N J0 0
      gpio_set PE_BUFF_OE_0_N B4 0
      gpio_set PE_BUFF_OE_1_N B5 0
   else
      gpio_set PE_BUFF_OE_0_N B4 1
      gpio_set PE_BUFF_OE_1_N B5 1
   fi
else
   gpio_set PE_BUFF_OE_0_N B4 1
   gpio_set PE_BUFF_OE_1_N B5 1
fi

if [ $(is_server_prsnt 3) == "1" ] && [ $(is_server_prsnt 4) == "1" ] ; then
   if [ $(get_slot_type 3) != "0" ] && [ $(get_slot_type 4) == "0" ] ; then
      gpio_set CLK_BUFF2_PWR_EN_N J1 0
      gpio_set PE_BUFF_OE_2_N B6 0
      gpio_set PE_BUFF_OE_3_N B7 0
   else
      gpio_set PE_BUFF_OE_2_N B6 1
      gpio_set PE_BUFF_OE_3_N B7 1
   fi
else
   gpio_set PE_BUFF_OE_2_N B6 1
   gpio_set PE_BUFF_OE_3_N B7 1
fi

# create devices on I2C bus 1 and bus 5
# Bus1,  Glacier Point
if [[ $(is_server_prsnt 1) == "1" && $(get_slot_type 1) != "0" && $(get_slot_type 1) != "4" ]]; then

   devmem 0x1e78a084 w 0xFFF77304

   if [ $(get_slot_type 1) == "2" ]; then
      # I2C mux, 0xE2
      echo pca9551 0x71 > /sys/class/i2c-dev/i2c-1/device/new_device
      # I2C GPIO, 0x40
      echo pca9551 0x20 > /sys/class/i2c-dev/i2c-1/device/new_device
   fi

   # EEPROM, 0xA2
   echo 24c128 0x51 > /sys/class/i2c-dev/i2c-1/device/new_device
   # Inlet temp sensor, 0x9A
   echo tmp75 0x4d > /sys/class/i2c-dev/i2c-1/device/new_device
   # outlet temp sensor, 0x9C
   echo tmp75 0x4e > /sys/class/i2c-dev/i2c-1/device/new_device
   # Voltage sensor, 0x80
   echo ina230 0x40 > /sys/class/i2c-dev/i2c-1/device/new_device
else
   devmem 0x1e78a084 w 0xFFF5E700
fi

# Bus5,  Glacier Point
if [[ $(is_server_prsnt 3) == "1" && $(get_slot_type 3) != "0" && $(get_slot_type 3) != "4" ]]; then

   devmem 0x1e78a184 w 0xFFF77304

   if [ $(get_slot_type 3) == "2" ]; then
      # I2C mux, 0xE2
      echo pca9551 0x71 > /sys/class/i2c-dev/i2c-5/device/new_device
      # I2C GPIO, 0x40
      echo pca9551 0x20 > /sys/class/i2c-dev/i2c-5/device/new_device
   fi

   # EEPROM, 0xA2
   echo 24c128 0x51 > /sys/class/i2c-dev/i2c-5/device/new_device
   # Inlet temp sensor, 0x9A
   echo tmp75 0x4d > /sys/class/i2c-dev/i2c-5/device/new_device
   # outlet temp sensor, 0x9C
   echo tmp75 0x4e > /sys/class/i2c-dev/i2c-5/device/new_device
   # Voltage sensor, 0x80
   echo ina230 0x40 > /sys/class/i2c-dev/i2c-5/device/new_device
else
   devmem 0x1e78a184 w 0xFFF5E700
fi

echo "done"
