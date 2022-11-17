#!/bin/bash

# shellcheck disable=SC1091
 . /usr/local/bin/openbmc-utils.sh
 . /usr/local/fbpackages/utils/ast-functions

has_mux=`i2cdetect -y 0 | grep -i "70: 70"`

probe_cpu0_dev() {
  i2c_device_add 0 0x3C sbrmi
  i2c_device_add 0 0x4C sbtsi
}

probe_cpu1_dev() {
  i2c_device_add 0 0x38 sbrmi
  i2c_device_add 0 0x48 sbtsi
}

probe_pwon_dev() {
  echo "Detect Power On..."
  PWRON="1"

  while true; do
    sleep 5
    if [ "$(gpio_get FM_PWRGD_CPU1_PWROK)" -eq $PWRON ]; then
      if [ -n "$has_mux" ]; then
        #init mux
        i2cset -f -y 0 0x70 0x46 0x01
        i2cset -f -y 0 0x70 0x40 0xc0
        i2cset -f -y 0 0x70 0x41 0xc0
        i2cget -f -y 0 0x38 0x01
        probe_cpu0_dev
        probe_cpu1_dev
        kv set apml_mux 1
      else
        gpio_set FM_APML_MUX2_SEL_R 0
        sleep 1
        i2cget -f -y 0 0x3C 0x01
        probe_cpu0_dev

        gpio_set FM_APML_MUX2_SEL_R 1
        sleep 1
        i2cget -f -y 0 0x38 0x01
        probe_cpu1_dev
        kv set apml_mux 0
      fi
      break
    fi
  done
  echo Done.
  sv restart sensord
}

probe_pwon_dev &
