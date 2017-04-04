#!/bin/bash
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.

# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
source /usr/local/bin/openbmc-utils.sh

function set_led_color
{
  local component=$1 color=$2

  if [ "$color" = 'yellow' ]; then
    echo out > /tmp/gpionames/${component}_RED/direction
    echo 0 > /tmp/gpionames/${component}_RED/value

    echo out > /tmp/gpionames/${component}_GREEN/direction
    echo 0 > /tmp/gpionames/${component}_GREEN/value

    echo in > /tmp/gpionames/${component}_BLUE/direction
    echo 1 > /tmp/gpionames/${component}_BLUE/value

  elif [ "$color" = 'red' ]; then
    echo out > /tmp/gpionames/${component}_RED/direction
    echo 0 > /tmp/gpionames/${component}_RED/value

    echo in > /tmp/gpionames/${component}_GREEN/direction
    echo 1 > /tmp/gpionames/${component}_GREEN/value

    echo in > /tmp/gpionames/${component}_BLUE/direction
    echo 1 > /tmp/gpionames/${component}_BLUE/value

  else
    echo in > /tmp/gpionames/${component}_RED/direction
    echo 1 > /tmp/gpionames/${component}_RED/value

    echo in > /tmp/gpionames/${component}_GREEN/direction
    echo 1 > /tmp/gpionames/${component}_GREEN/value

    echo out > /tmp/gpionames/${component}_BLUE/direction
    echo 0 > /tmp/gpionames/${component}_BLUE/value
  fi
}
# System led:
# If atleast 1 card is not present = yellow (red+green)
# otherwise = blue
# note presence util reads peer cmm =0
function set_sys_led
{
  local ret=0
  ret=$('/usr/local/bin/presence_util.sh' | grep ": 0" | wc -l)
  if [ $ret -gt 1 ]; then
    set_led_color 'SYS_LED' 'yellow'
  else
    set_led_color 'SYS_LED' 'blue'
  fi
}

# Fabric LED:
# all present = blue
# atleast 1 missing = yellow
# more than 1 missing = red
function set_fab_led
{
  local count=0
  count=$(/usr/local/bin/presence_util.sh | sed -n "/^fc.*: 0/p" | wc -l)
  if [ $count -eq 1 ]; then
    set_led_color 'FAB_LED' 'yellow'
  elif [ $count -gt 1 ]; then
    set_led_color 'FAB_LED' 'red'
  else
    set_led_color 'FAB_LED' 'blue'
  fi
}

function check_psu_alarm
{
  local sensor=$1
  local vout_low_th=11 vout_high_th=13 vin_low_th=100 vin_high_th=250 vin=0 vout=0

  readarray pfe1 < <(/usr/bin/sensors $sensor | grep -e vin -e vout1)
  vin=$(echo "${pfe1}" | sed -e 's/vin:\s*+\(.*\)[.].*$/\1/')
  vout=$(echo "${pfe1[1]}" | sed -e 's/vout1:\s*+\(.*\)[.].*$/\1/')
  if [ $vin -lt $vin_low_th ] || [ $vin -gt $vin_high_th ] ; then
    return 0
  fi
  if [ $vout -lt $vout_low_th ] || [ $vout -gt $vout_high_th ]; then
    return 0
  fi
  return 1
}

function set_psu_led
{
  local ret=0
  sensors=( "pfe3000-i2c-24-10" "pfe3000-i2c-25-10" "pfe3000-i2c-26-10" "pfe3000-i2c-27-10" )
  for i in "${sensors[@]}"
  do
    if check_psu_alarm $i; then
      set_led_color 'PSU_LED' 'red'
      return
    fi
  done
  ret=$(/usr/local/bin/presence_util.sh | sed -n "/psu.*: 0/p" | wc -l)
  if [ $ret -gt 0 ]; then
    set_led_color 'PSU_LED' 'yellow'
  else
    set_led_color 'PSU_LED' 'blue'
  fi
}

function set_fan_led
{
  local ret=0
  ret=$(/usr/bin/sensors fancpld-* | sed -n "/^Fan.*: 0 RPM/p" | wc -l)
  if [ $ret -gt 0 ]; then
    set_led_color 'FAN_LED' 'red'
    return
  fi
  ret=$(/usr/local/bin/presence_util.sh | sed -n "/fan.*: 0/p" | wc -l)
  if [ $ret -gt 0 ]; then
    set_led_color 'FAN_LED' 'yellow'
  else
    set_led_color 'FAN_LED' 'blue'
  fi
}

while :
do
  set_sys_led
  sleep 1s
  set_fab_led
  sleep 1s
  set_psu_led
  sleep 1s
  set_fan_led
  sleep 10m
done
