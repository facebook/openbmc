#!/bin/bash
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

### BEGIN INIT INFO
# Provides:          setup-por
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Power on Server
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

kv set mb_polling_status      1
kv set swb_polling_status     1
kv set nic0_polling_status    1
kv set nic1_polling_status    1
kv set scm_polling_status     1
kv set vpdb_polling_status    1
kv set hpdb_polling_status,   1
kv set fan_bp1_polling_status 1
kv set fan_bp2_polling_status 1

gpu_prsnt=$(cat /tmp/gpionames/GPU_PRSNT_N_ISO_R/value)
if [ "$gpu_prsnt" -eq 0 ]; then
  GPU_CONFIG="gpu_config"
  gpu=`kv get $GPU_CONFIG persistent`
  if [ "$gpu" == "hgx" ]; then
    kv set hgx_polling_status   1
    kv set ubb_polling_status   0
  elif [ "$gpu" == "ubb" ]; then
    sed -i '2s/hgx/ubb/' /etc/sv/sensord/run
    kv set hgx_polling_status   0
    kv set ubb_polling_status   1
  fi
fi
