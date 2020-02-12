#!/bin/sh

# TODO:
# 	Remove this workwround at DVT stage

. /usr/local/fbpackages/utils/ast-functions

P3V3=$(sensors iio_hwmon-isa-0000 | sed -n '8p' | awk -F '+' '{print $2}' | cut -d " " -f 1)
if [[ $P3V3 > 3.0 ]]; then
  gpio_set SEL0_CLK_MUX 0
  gpio_set SEL1_CLK_MUX 1

  gpio_set OEA_CLK_MUX_N 0
  gpio_set OEB_CLK_MUX_N 1
fi
