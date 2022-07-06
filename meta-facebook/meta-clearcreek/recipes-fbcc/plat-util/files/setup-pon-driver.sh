#!/bin/bash
. /usr/local/bin/openbmc-utils.sh
. /usr/local/fbpackages/utils/ast-functions

gp_sys_pwr_good=$(gpio_get SYS_PWR_READY GPIOB3)
if [ $gp_sys_pwr_good -eq 1 ]; then
  echo "Setup TMP422 driver"
  i2c_device_add 6 0x4d tmp422
  i2c_device_add 6 0x4e tmp422

  echo "Setup VR sensor driver"

  gp_board_id0=$(gpio_get BOARD_ID0 GPION5)
  gp_board_id1=$(gpio_get BOARD_ID1 GPION6)
  gp_board_id2=$(gpio_get BOARD_ID2 GPION7)

  if [ $gp_board_id0 -eq 1 ] && [ $gp_board_id1 -eq 1 ] && [ $gp_board_id2 -eq 0 ]; then
    i2c_device_add 5 0x34 mpq8645p
    i2c_device_add 5 0x36 mpq8645p
    i2c_device_add 5 0x60 isl69260
    i2c_device_add 5 0x61 isl69260
    i2c_device_add 5 0x72 isl69260
    i2c_device_add 5 0x75 isl69260
  else
    i2c_device_add 5 0x30 mpq8645p
    i2c_device_add 5 0x31 mpq8645p
    i2c_device_add 5 0x32 mpq8645p
    i2c_device_add 5 0x33 mpq8645p
    i2c_device_add 5 0x34 mpq8645p
    i2c_device_add 5 0x35 mpq8645p
    i2c_device_add 5 0x36 mpq8645p
    i2c_device_add 5 0x3b mpq8645p
  fi

  sv restart sensord
  kv set pon_driver_loaded on
fi
