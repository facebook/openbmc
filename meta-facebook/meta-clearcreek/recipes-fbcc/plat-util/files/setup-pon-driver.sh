#!/bin/bash
. /usr/local/bin/openbmc-utils.sh
. /usr/local/fbpackages/utils/ast-functions

gp_sys_pwr_good=$(gpio_get SYS_PWR_READY GPIOB3)
if [ $gp_sys_pwr_good -eq 1 ]; then
  echo "Setup TMP422 driver"
  i2c_device_add 6 0x4d tmp422
  i2c_device_add 6 0x4e tmp422

  echo "Setup VR sensor driver"
  i2c_device_add 5 0x30 mpq8645p
  i2c_device_add 5 0x31 mpq8645p
  i2c_device_add 5 0x32 mpq8645p
  i2c_device_add 5 0x33 mpq8645p
  i2c_device_add 5 0x34 mpq8645p
  i2c_device_add 5 0x35 mpq8645p
  i2c_device_add 5 0x36 mpq8645p
  i2c_device_add 5 0x3b mpq8645p
  sv restart sensord
  kv set pon_driver_loaded on
fi
