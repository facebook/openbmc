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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh
. /usr/local/fbpackages/utils/ast-functions
KV_CMD="/usr/bin/kv"

GPU_CONFIG="gpu_config"
HGX_MODEL="hgx_model"

HGX_FRU_BIN="/tmp/fruid_hgx.bin"
HGX_EEPROM_ADDR="0x53"
UBB_FRU_BIN="/tmp/fruid_ubb.bin"
UBB_EEPROM_ADDR="0x54"

probe_eeprom_driver () {
  addr_wo_prefix=${1#0x}
  MAX_RETRY=$2
  driver="24c64"

  for (( i=1; i<=$MAX_RETRY; i++ )); do
    if [ ! -L "/sys/bus/i2c/drivers/at24/9-00$addr_wo_prefix" ]; then
      i2c_device_delete 9 $1 2>/dev/null
      i2c_device_add 9 $1 "$driver" 2>/dev/null
    else
      return
    fi
    sleep 1
  done
}

copy_gpu_eeprom () {
  addr=$1
  bin=$2
  MAX_RETRY=$3

  for (( i=1; i<=$MAX_RETRY; i++ )); do
    if [ ! -e "$bin" ] || [ $(wc -c <"$bin") -eq 0 ]; then
      /bin/dd if=/sys/class/i2c-dev/i2c-9/device/9-00${addr}/eeprom of=$bin bs=512 count=1
    fi
    sleep 0.2
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

setup_gpu_model() {
  hmc_model=("H100" "H200" "B100" "B200")
  # TODO add UBB model
  gpu=$1

  if [ "$gpu" == "hgx" ]; then
    for i in "${!hmc_model[@]}"; do
      is_model="$(strings "$HGX_FRU_BIN" | grep -i "${hmc_model[$i]}")"
      if [ -n "$is_model" ]; then
        $KV_CMD set $GPU_MODEL "${hmc_model[$i]}" persistent
        return
      fi
    done
  elif [ "$gpu" == "ubb" ]; then
    board_num=$(fruid-util ubb | grep "Board Part Number" | awk '{print $NF}')
    custom_data1=$(fruid-util ubb | grep "Product Custom Data 1" | awk '{print $NF}')
    fruid-util ubb --modify --BPN $custom_data1 /tmp/fruid_ubb.bin > /dev/null 2>&1
    fruid-util ubb --modify --PPN $custom_data1 /tmp/fruid_ubb.bin > /dev/null 2>&1
    fruid-util ubb --modify --PCD1 $board_num   /tmp/fruid_ubb.bin > /dev/null 2>&1
  fi
}

setup_gpu_eeprom () {
  gpu=("hgx" "ubb")
  names=("NVIDIA" "AMD")
  snr_polling=("hgx_polling_status" "ubb_polling_status")
  addr=("$HGX_EEPROM_ADDR" "$UBB_EEPROM_ADDR")
  bins=("$HGX_FRU_BIN" "$UBB_FRU_BIN")
  MAX_RETRY=10

  for loop in "${!addr[@]}"; do
    probe_eeprom_driver "${addr[$loop]}" $MAX_RETRY
    copy_gpu_eeprom "${addr[$loop]#0x}" "${bins[$loop]}" $MAX_RETRY
    is_gpu="$(strings "${bins[$loop]}" | grep -i "${names[$loop]}")"
    if [ -n "$is_gpu" ]; then
      $KV_CMD set $GPU_CONFIG "${gpu[$loop]}" persistent
      $KV_CMD set "${snr_polling[$loop]}" 1
      setup_gpu_model "${gpu[$loop]}"
      gpu_snr_mon "${gpu[$loop]}" enable
      return
    fi
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
