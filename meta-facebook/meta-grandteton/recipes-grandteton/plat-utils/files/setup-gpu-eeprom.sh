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
HGX_EEPROM_ADDR="0x53"
UBB_FRU_BIN="/tmp/fruid_ubb.bin"
UBB_EEPROM_ADDR="0x54"

probe_eeprom_driver () {
  addr_wo_prefix=${1#0x}
  MAX_RETRY=$2
  for (( i=1; i<=$MAX_RETRY; i++ )); do
    if [ ! -L "/sys/bus/i2c/drivers/at24/9-00$addr_wo_prefix" ]; then
      i2c_device_delete 9 $1 2>/dev/null
      i2c_device_add 9 $1 24c64 2>/dev/null
    else
      return
    fi
    sleep 3
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

gpu_snr_mon () {
  gpu_config=$1
  snr_mon=$2

  if [ "$snr_mon" == "enable" ]; then
    if [ -z "$(cat /etc/sv/sensord/run | grep "$gpu_config")" ]; then
      sed -i "2 s/$/ $gpu_config/" /etc/sv/sensord/run
    fi

    # If sensord didn't monitor the gpu, then restart to monitor it
    if [ -z "$(ps | grep sensord | grep "$gpu_config")" ]; then
      sv restart sensord
    fi
  else
    if [ -n "$(cat /etc/sv/sensord/run | grep "$gpu_config")" ]; then
      sed -i "2 s/ $gpu_config//g" /etc/sv/sensord/run
    fi

    # If sensord is monitoromg the gpu, then stop to monitor it
    if [ -n "$(ps | grep sensord | grep "$gpu_config")" ]; then
      sv restart sensord
    fi
  fi
}

setup_gpu_eeprom () {
  gpu=("hgx" "ubb")
  names=("NVIDIA" "AMD")
  snr_polling=("hgx_polling_status" "ubb_polling_status")
  addr=("$HGX_EEPROM_ADDR" "$UBB_EEPROM_ADDR")
  bins=("$HGX_FRU_BIN" "$UBB_FRU_BIN")

  MAX_RETRY=10

for count in $(seq 1 $MAX_RETRY); do
  for loop in "${!addr[@]}"; do
    response=$(i2cget -y -f 9 "${addr[$loop]}" 0x00)
    if [[ $? -eq 0 && ! "$response" =~ "Error" ]]; then
      probe_eeprom_driver "${addr[$loop]}" $MAX_RETRY
      copy_gpu_eeprom "${addr[$loop]#0x}" "${bins[$loop]}"
      is_gpu="$(strings "${bins[$loop]}" | grep -i "${names[$loop]}")"
      if [ -n "$is_gpu" ]; then
        $KV_CMD set $GPU_CONFIG "${gpu[$loop]}" persistent
        $KV_CMD set "${snr_polling[$loop]}" 1
        gpu_snr_mon "${gpu[$loop]}" enable
        return 0
      fi
    else
      sleep 0.2
    fi
  done
done

  $KV_CMD set $GPU_CONFIG "unknown" persistent
  gpu_snr_mon hgx disable
  gpu_snr_mon ubb disable
  /usr/bin/logger -t "gpiod" -p daemon.crit "Detecting an unknown GPU"
}

LOCK_FILE="/tmp/gpu_fpga.lock"
if [ -e "$LOCK_FILE" ]; then
  exit 1
else
  touch "$LOCK_FILE"

  setup_gpu_eeprom

  rm "$LOCK_FILE"
fi
