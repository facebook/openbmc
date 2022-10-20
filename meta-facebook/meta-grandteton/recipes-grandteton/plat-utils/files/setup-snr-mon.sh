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

### BEGIN INIT INFO
# Provides:          setup-por
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Power on Server
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh
. /usr/local/fbpackages/utils/ast-functions

mb_hsc=$(($(gpio_get FM_BOARD_BMC_SKU_ID3) << 1 |
          $(gpio_get FM_BOARD_BMC_SKU_ID2)))

MB_HSC_SECOND="1"  # ltc4282/ltc4286

if [ "$mb_hsc" -eq "$MB_HSC_SECOND" ]; then
  sed -i "2{s/$/ hsc/}" /etc/sv/sensord/run
fi

fw-util swb --version pex01_vcc > /dev/null 2>&1
SWB_HSC_MODULE="1"  # ltc4282/ltc4286
swb_hsc=$(kv get swb_hsc_module)
if [ "$swb_hsc" -eq "$SWB_HSC_MODULE" ]; then
  sed -i "2{s/$/ swb_hsc/}" /etc/sv/sensord/run
fi
