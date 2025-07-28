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

setup_gpu_pldmd() {
  if [ -n "$(pgrep -f "/usr/local/bin/pldmd --bus 11")" ]; then
    sv stop pldmd_11
  fi

  if [ -n "$(pgrep -f "/usr/local/bin/mctpd smbus 11 0x10")" ]; then
    sv stop mctpd_11
  fi

  echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-11/new_device
  mctp-util -d 11 0xae 0x00 0x00 0x00 0x2
  mctp-util -d 11 0xae 0x00 0x00 0x00 0x1 0x0 0x24
  runsv /etc/sv/mctpd_11 > /dev/null 2>&1 &
  sleep 3
  if [ -z "$(pgrep -f "/usr/local/bin/mctpd smbus 11 0x10")" ]; then
    sv start mctpd_11
  fi

  runsv /etc/sv/pldmd_11 > /dev/null 2>&1 &
  sleep 3
  if [ -z "$(pgrep -f "/usr/local/bin/pldmd --bus 11")" ]; then
    sv start pldmd_11
  fi
}

LOCK_FILE="/tmp/gpu_pldmd.lock"
if [ -e "$LOCK_FILE" ]; then
  exit 1
else
  touch "$LOCK_FILE"

  # Check if pldmd is ready
  if ! pldmd-util -b 11 -e 0x24 raw 0x02 0x21 0xC0 0x03 0x00 0x00; then
    setup_gpu_pldmd
  fi

  rm "$LOCK_FILE"
fi
