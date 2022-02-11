#!/bin/sh
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
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

. /usr/local/fbpackages/utils/ast-functions

#TODO: Add logic to control mux based on front panel switch
echo -n "Set USB Mux to given slot ... "

# USB_MUX_SEL signals: GPIOE4(36), GPIOE5(37)
slot=$1

case $slot in
  1)
    gpio_set FM_USB_SW0 E4 0
    gpio_set FM_USB_SW1 E5 0
    gpio_set USB_MUX_EN_R_N AB3 0
    ;;
  2)
    gpio_set FM_USB_SW0 E4 E4 1
    gpio_set FM_USB_SW1 E5 E5 0
    gpio_set USB_MUX_EN_R_NAB3 0
    ;;
  3)
    gpio_set FM_USB_SW0 E4 E4 0
    gpio_set FM_USB_SW1 E5 E5 1
    gpio_set USB_MUX_EN_R_N AB3 0
    ;;
  4)
    gpio_set FM_USB_SW0 E4 E4 1
    gpio_set FM_USB_SW1 E5 E5 1
    gpio_set USB_MUX_EN_R_N AB3 0
    ;;
  *)
    gpio_set USB_MUX_EN_R_N AB3 1
    ;;
esac

echo "Done"
