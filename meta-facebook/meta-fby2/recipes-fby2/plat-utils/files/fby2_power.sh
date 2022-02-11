#!/bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

. /usr/local/fbpackages/utils/ast-functions

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

LPS_PATH=/mnt/data/power/por/last_state

prog="$0"

usage() {
    echo "Usage: $prog <slot#> <command> [command options]"
    echo
    echo "Commands:"
    echo "  status: Get the current 1S server power status"
    echo
    echo "  on: Power on 1S server if not powered on already"
    echo "    options:"
    echo "      -f: Re-do power on sequence no matter if 1S server has "
    echo "          been powered on or not."
    echo
    echo "  off: Power off 1S server ungracefully"
    echo
    echo
}

do_status() {
    if [ $(is_server_prsnt $slot) == "0" ]; then
      echo  "The given slot is Empty"
      return 0
    fi

    if [[ $(get_slot_type $slot) == "1" || $(get_slot_type $slot) == "2" ]]; then
      echo "The given slot type is not server"
      return 0
    fi

    echo -n "1S Server power for slot#$slot is "
    if [ $(fby2_is_server_on $slot) -eq 1 ] ; then
        echo "on"
    else
        echo "off"
    fi
    return 0
}

do_on() {

    if [ $(is_server_prsnt $slot) == "0" ]; then
      echo "The given slot is Empty"
      return 0
    fi

    if [[ $(get_slot_type $slot) == "1" || $(get_slot_type $slot) == "2" ]]; then
      echo "The given slot type is not server"
      return 0
    fi

    local force opt
    force=0
    while getopts "f" opt; do
        case $opt in
            f)
                force=1
                ;;
            *)
                usage
                exit -1
                ;;

        esac
    done
    echo -n "Power on slot#$slot server ..."
    if [ $force -eq 0 ]; then
        # need to check if 1S Server is on or not
        if [ $(fby2_is_server_on $slot) -eq 1 ]; then
            echo " Already on. Skip!"
            return 1
        fi
    fi

    # TODO: State the power state change
    echo "on $(date +%s)" > $LPS_PATH

    # disable GPIOD pass-though function
    devmem_set_bit $(scu_addr 7c) 21

    # first make sure, GPIO is high
    gpio_set $gpio_name $gpio 1
    # generate the power on pulse
    gpio_set $gpio_name $gpio 0
    sleep 1
    gpio_set $gpio_name $gpio 1
    sleep 1
    # Turn on the power LED
    /usr/local/bin/power_led.sh $slot on

    # enable GPIOD pass-though function
    devmem_set_bit $(scu_addr 70) 21

    echo " Done"
    return 0
}

do_off() {
    if [ $(is_server_prsnt $slot) == "0" ]; then
      echo "The given slot is Empty"
      return 0
    fi

    if [[ $(get_slot_type $slot) == "1" || $(get_slot_type $slot) == "2" ]]; then
      echo "The given slot type is no server"
      return 0
    fi

    echo -n "Power off slot#$slot server ..."

    #TODO: State the power state change
    echo "off $(date +%s)" > $LPS_PATH

    # disable GPIOD pass-though function
    devmem_set_bit $(scu_addr 7c) 21

    # first make sure, GPIO is high
    gpio_set $gpio_name $gpio 1
    sleep 1
    gpio_set $gpio_name $gpio 0
    sleep 5
    gpio_set $gpio_name $gpio 1
    # Turn off the power LED
    /usr/local/bin/power_led.sh $slot off

    # enable GPIOD pass-though function
    devmem_set_bit $(scu_addr 70) 21

    echo " Done"
    return 0
}

# Slot1: GPIOD1(25), Slot2: GPIOD3(27), Slot3: GPIOD5(29), Slot4: GPIOD7(31)
slot=$1

case $slot in
  1)
    gpio=D1
    gpio_name=PWR_SLOT1_BTN_N
    ;;
  2)
    gpio=D3
    gpio_name=PWR_SLOT2_BTN_N
    ;;
  3)
    gpio=D5
    gpio_name=PWR_SLOT3_BTN_N
    ;;
  4)
    gpio=D7
    gpio_name=PWR_SLOT4_BTN_N
    ;;
  *)
    gpio=D1
    gpio_name=PWR_SLOT1_BTN_N
    ;;
esac

command="$2"
shift
shift

case "$command" in
    status)
        do_status $@
        ;;
    on)
        do_on $@
        ;;
    off)
        do_off $@
        ;;
    *)
        usage
        exit -1
        ;;
esac

exit $?
