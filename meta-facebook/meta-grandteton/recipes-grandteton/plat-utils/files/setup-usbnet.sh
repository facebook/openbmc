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
. /usr/local/fbpackages/utils/ast-functions
KV_CMD="/usr/bin/kv"

setup_usb_hub () {
  MAX_RETRY=10
  for (( i=1; i<=$MAX_RETRY; i++ )); do
    ifconfig usb0 192.168.31.2 netmask 255.255.0.0
    if [ $? -eq 0 ]; then
      $KV_CMD set is_usbnet_ready 1
      break
    fi
  done
}

setup_gpu_config() {
  HGX_EEPROM_ADDR="53"
  i2cget -y -f 9 0x$HGX_EEPROM_ADDR 0x00 2>/dev/null
  if [ $? -eq 0 ]; then
    $KV_CMD set $GPU_CONFIG hgx persistent
  else
    $KV_CMD set $GPU_CONFIG ubb persistent
  fi
}

LOCK_FILE="/tmp/usbhub.lock"
if [ -e "$LOCK_FILE" ]; then
  exit 1
else
  touch "$LOCK_FILE"

  setup_usb_hub
  setup_gpu_config

  rm "$LOCK_FILE"
fi
