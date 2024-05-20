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

GPU_CONFIG="gpu_config"
HGX_FRU_BIN="/tmp/fruid_hgx.bin"
HGX_EEPROM_ADDR="53"
UBB_FRU_BIN="/tmp/fruid_ubb.bin"
UBB_EEPROM_ADDR="54"

probe_eeprom_driver () {
  addr16="0x$1"
  MAX_RETRY=$2
  for (( i=1; i<=$MAX_RETRY; i++ )); do
    if [ ! -L "/sys/bus/i2c/drivers/at24/9-00$1" ]; then
      i2c_device_delete 9 0x$1 2>/dev/null
      i2c_device_add 9 $addr16 24c64 2>/dev/null
    else
      return
    fi
    sleep 5
  done
  /usr/bin/logger -t "debug" -p daemon.crit "Failed to dump GPU EEPROM"
}

copy_gpu_eeprom () {
  addr=$1
  bin=$2
  for (( i=1; i<=$MAX_RETRY; i++ )); do
    if [ ! -e "$bin" ]; then
      /bin/dd if=/sys/class/i2c-dev/i2c-9/device/9-00${addr}/eeprom of=$bin bs=512 count=1
    fi
  done
}

setup_gpu_eeprom () {
  gpu=`$KV_CMD get $GPU_CONFIG persistent`
  if [ "$gpu" == "hgx" ]; then
    MAX_RETRY=10
    probe_eeprom_driver $HGX_EEPROM_ADDR $MAX_RETRY
    addr=$HGX_EEPROM_ADDR
    bin=$HGX_FRU_BIN
  elif [ "$gpu" == "ubb" ]; then
    MAX_RETRY=144
    probe_eeprom_driver $UBB_EEPROM_ADDR $MAX_RETRY
    addr=$UBB_EEPROM_ADDR
    bin=$UBB_FRU_BIN
  else
    /usr/bin/logger -t "debug" -p daemon.crit "Detecting an unknown GPU"
    exit 1
  fi

  copy_gpu_eeprom $addr $bin
}

LOCK_FILE="/tmp/gpu_fpga.lock"
if [ -e "$LOCK_FILE" ]; then
  exit 1
else
  touch "$LOCK_FILE"

  setup_gpu_eeprom

  rm "$LOCK_FILE"
fi
