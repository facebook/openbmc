#!/bin/sh
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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

. /usr/local/bin/openbmc-utils.sh

MAX_RETRY=30
retry=0
usb_pst=""

gp_pst=$(gpio_get GPU_CABLE_PRSNT_D_N)

setup_usb_dev() {
  if [ $gp_pst -eq 0 ]; then
    echo "Init usbnet ethernet for HMC"

    while [ $retry -lt "$MAX_RETRY" ]
    do 
      ifconfig usb0 192.168.31.2 netmask 255.255.0.0
 
      usb_pst=$(ifconfig -a | grep usb0)
      if [ "$usb_pst" != "" ]; then
        echo "Setup usbnet ethernet device successfully"
        exit 0
      else
        retry=$(($retry+1))
        sleep 10
      fi
    done

    echo "Can't setup usbnet ethernet device for HMC"
  fi
}

setup_usb_dev &
