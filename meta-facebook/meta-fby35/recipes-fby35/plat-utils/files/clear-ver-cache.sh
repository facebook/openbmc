#! /bin/bash
#
# Copyright 2023-present Facebook. All Rights Reserved.
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

# shellcheck source=common/recipes-utils/openbmc-utils/files/openbmc-utils.sh
. /usr/local/bin/openbmc-utils.sh

clear_vr_cache() {
  local slot=$1
  local list
  local cache

  if list=$(ls /mnt/data/kv_store/slot"${slot}"_*vr_*_crc 2>/dev/null); then
    mapfile list <<< "$list"
    for cache in "${list[@]}"; do
      /usr/bin/kv del "$(basename "$cache")" persistent
    done
  fi
}

if [ "$(is_bmc_por)" -eq 1 ]; then
  for i in {1..4}; do
    clear_vr_cache $i
  done
fi
