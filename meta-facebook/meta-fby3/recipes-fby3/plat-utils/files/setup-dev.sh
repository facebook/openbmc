#! /bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
. /usr/local/fbpackages/utils/ast-functions

HSC_DET_ADM1278=0
HSC_DET_LTC4282=1
HSC_DET_MP5990=2
HSC_DET_ADM1276=3

function create_new_dev() {
  local dev_name=$1
  local addr=$2
  local bus=$3

  echo $dev_name $addr > /sys/class/i2c-dev/i2c-${bus}/device/new_device
}

function init_class1_dev(){
  hsc_detect=$(get_hsc_bb_detect)
  if [ $hsc_detect -eq $HSC_DET_ADM1278 ]; then
    for i in $(seq 1 5)
    do
      #enable the register of the temperature of hsc
      /usr/sbin/i2cset -y 11 0x40 0xd4 0x1c 0x3f i 2> /dev/null
      RET=$?
      if [ "$RET" = 0 ]; then
        break
      fi
    done
  elif [ $hsc_detect -eq $HSC_DET_LTC4282 ]; then
    create_new_dev "ltc4282" 0x40 11
    create_new_dev "tmp401" 0x4c 12
  elif [ $hsc_detect -eq $HSC_DET_ADM1276 ]; then
    create_new_dev "tmp401" 0x4c 12
  fi

  #create the device of the inlet/outlet temp.
  create_new_dev "lm75" 0x4e 12
  create_new_dev "lm75" 0x4f 12

  #create the device of bmc/bb fru.
  create_new_dev "24c128" 0x51 11 
  create_new_dev "24c128" 0x54 11

  local VENDOR_MPS="0x4d5053"
  local chip=""

  #create the device of medusa board
  vendor=$(/usr/sbin/i2cget -y 11 0x44 0x99 i 2> /dev/null | tail -n 1)
  if [ "$vendor" == "$VENDOR_MPS" ]; then
    chip="mp5920"
  else
    chip="ltc4282"
  fi

  create_new_dev $chip 0x44 11

  # /mnt/data/kv_store checking, and create one if not exist
  if [ ! -d /mnt/data/kv_store ]; then
    /bin/mkdir -p /mnt/data/kv_store
  fi
  echo -n $chip > /mnt/data/kv_store/bb_hsc_conf

  # check nic power to see if it's need to be set to standby mode
  set_nic_power
}

function init_class2_dev(){
  #create the device of the outlet temp. 
  create_new_dev "lm75" 0x4f 2

  #create the device of bmc/nic fru.
  create_new_dev "24c128" 0x51 10
  create_new_dev "24c128" 0x54 10
}

#create the device of mezz card
create_new_dev "tmp421" 0x1f 8
create_new_dev "24c32" 0x50 8

echo -n "Setup devs for fby3..."

bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  #The BMC of class2
  init_class2_dev
elif [ $bmc_location -eq 14 ] || [ $bmc_location -eq 7 ]; then
  #The BMC of class1
  init_class1_dev
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

echo "Done."
