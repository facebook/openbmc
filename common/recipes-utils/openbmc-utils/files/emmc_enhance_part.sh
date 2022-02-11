#!/bin/sh
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

show_usage() {
  echo "Error  : invalid command line arguments!"
  echo "Usage  : $0 <mmc_device>"
  echo "Example: $0 /dev/mmcblk0"
  exit -1
}
if [[ $# -ne 1 ]] || ! [[ -e $1 ]]; then
  show_usage
fi
MMC_DEVICE=$1

umount /mnt/data1 > /dev/null

emmc_config_extcsd() {
  # Configure pSLC mode
  vendor=$(mmc cid read /sys/class/mmc_host/mmc0/mmc0\:0001/ | grep manufacturer | cut -d " " -f 2)
  if [[ "$vendor" = "'Micron'" ]]; then
    # Refer Micron document TN-FC-40
    val=$(mmc extcsd read $MMC_DEVICE | grep PARTITIONING_SUPPORT)
    val=${val:0-4}
    partitioning_sup=$((val%2))
    enhanced_sup=$((val/2%2))
    if [[ "$partitioning_sup" != "1" ]] || [[ "$enhanced_sup" != "1" ]]; then
      echo "This eMMC don't support partitioning."
      return
    fi

    completed=$(mmc extcsd read $MMC_DEVICE | grep PARTITION_SETTING_COMPLETED)
    completed=${completed:0-4}
    if [[ "$completed" == "0x01" ]]; then
      echo "Partitioning had been done"
      exit 0
    fi
    echo "Configure eMMC partitions"
    # Set ERASE_GROUP_DEF to 1
    mmcraw -o 175 -D 0x1 write-extcsd $MMC_DEVICE > /dev/null
    # Set ENH_START_ADDR to 0
    mmcraw -o 136 -D 0x0 write-extcsd $MMC_DEVICE > /dev/null
    mmcraw -o 137 -D 0x0 write-extcsd $MMC_DEVICE > /dev/null
    mmcraw -o 138 -D 0x0 write-extcsd $MMC_DEVICE > /dev/null
    mmcraw -o 139 -D 0x0 write-extcsd $MMC_DEVICE > /dev/null
    # Set ENH_SIZE_MULT to MAX_ENH_SIZE_MULT
    max_enh_size_mult=$(mmc extcsd read $MMC_DEVICE | grep MAX_ENH_SIZE_MULT)
    max_enh_size_mult0=0x${max_enh_size_mult:0-2}
    max_enh_size_mult1=0x${max_enh_size_mult:0-4:2}
    max_enh_size_mult2=0x${max_enh_size_mult:0-6:2}
    mmcraw -o 140 -D $max_enh_size_mult0 write-extcsd $MMC_DEVICE > /dev/null
    mmcraw -o 141 -D $max_enh_size_mult1 write-extcsd $MMC_DEVICE > /dev/null
    mmcraw -o 142 -D $max_enh_size_mult2 write-extcsd $MMC_DEVICE > /dev/null
    # Set ENH_USR to 1
    mmcraw -o 156 -D 0x1 write-extcsd $MMC_DEVICE > /dev/null
    # Set PARTITIONING_SETTING_COMPLETED to 1
    mmcraw -o 155 -D 0x1 write-extcsd $MMC_DEVICE > /dev/null
  else
    echo "The manufacturer $vendor is not supported"
  fi
  # Disable cache
  cache_ctl=$(mmc extcsd read $MMC_DEVICE | grep CACHE_CTRL)
  cache_ctl=${cache_ctl:0-4}
  if [[ $cache == "0x01" ]]; then
    echo "Disable eMMC cache"
    mmc cache disable > /dev/null
  fi
}
emmc_config_extcsd

echo "WARNING: Do chassis cycle to apply the enhancement"
