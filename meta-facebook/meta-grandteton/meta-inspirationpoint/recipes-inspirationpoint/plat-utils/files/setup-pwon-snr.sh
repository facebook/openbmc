#!/bin/bash

# shellcheck disable=SC1091
 . /usr/local/bin/openbmc-utils.sh
 . /usr/local/fbpackages/utils/ast-functions


probe_pwon_dev() {
  echo "Detect Power On..."
  PWRON="1"
  while true; do
    sleep 5
    if [ "$(gpio_get FM_PWRGD_CPU1_PWROK)" -eq $PWRON ]; then
      #Switch Mux
      i2cset -f -y 0 0x70 0x40 0xc0
      i2cset -f -y 0 0x70 0x41 0xc0
      #Prove SBRMI and SBTSI driver
      i2c_device_add 0 0x38 sbrmi
      i2c_device_add 0 0x3C sbrmi
      i2c_device_add 0 0x48 sbtsi
      i2c_device_add 0 0x4C sbtsi
      break
    fi
  done
  echo Done.
  sv restart sensord
}

probe_pwon_dev &
