#!/bin/sh
#
# Copyright 2021-present Facebook. All Rights Reserved.
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

I2CGET_CMD=/usr/sbin/i2cget
I2C13_LTC2990_ADDR=0x4c
I2C13_ADS1015_ADDR=0x49
I2C9_LTC2991_ADDR=0x48
I2C9_ADC128_ADDR=0x1d
DPB_FRU_PATH="/tmp/fruid_dpb.bin"
IOCM_FRU_PATH="/tmp/fruid_iocm.bin"
DPB_KEY="dpb_source_info"
IOCM_KEY="iocm_source_info"

UNKNOWN_SRC=0
MAIN_SRC=1
SECOND_SRC=2

FRU=$1

if [ "$FRU" = "dpb" ]; then
  I2C_BUS=9
  I2C_ADDR_MAIN=$I2C9_ADC128_ADDR
  I2C_ADDR_SECOND=$I2C9_LTC2991_ADDR
  FRU_PATH=$DPB_FRU_PATH
  KEY_SOURCE_INFO=$DPB_KEY
elif [ "$FRU" = "e1s_iocm" ]; then
  I2C_BUS=13
  I2C_ADDR_MAIN=$I2C13_ADS1015_ADDR
  I2C_ADDR_SECOND=$I2C13_LTC2990_ADDR
  FRU_PATH=$IOCM_FRU_PATH
  KEY_SOURCE_INFO=$IOCM_KEY
else
  logger -s -p user.warn -t check_2nd_src "Abort by invalid input"
  exit
fi

check_fru_content() {
  RET=$UNKNOWN_SRC
  RETRY=5
  while [ $RETRY -gt 0 ]
  do
    if [ -f $FRU_PATH ]; then
      PCD=$($FRUIDUTIL_CMD "$FRU" | grep "Product Custom Data")
      if echo "$PCD" | grep -q "Non-TI"; then
        echo $SECOND_SRC
        return 0
      else
        echo $MAIN_SRC
        return 0
      fi
    else
      RETRY=$((RETRY-1))
      sleep 1
    fi
  done
  logger -s -p user.warn -t check_2nd_src "Fail to read $FRU FRU: $FRU_PATH doesn't exist"
  echo $UNKNOWN_SRC
}

check_i2c_dev() {
  RET_MAIN=$($I2CGET_CMD -y -f $I2C_BUS $I2C_ADDR_MAIN 0 2>&1 1>/dev/null)
  RET_SECOND=$($I2CGET_CMD -y -f $I2C_BUS $I2C_ADDR_SECOND 0 2>&1 1>/dev/null)

  if ! echo "$RET_MAIN" | grep -q "Read failed"; then
    echo $MAIN_SRC
  elif ! echo "$RET_SECOND" | grep -q "Read failed"; then
     echo $SECOND_SRC
  else
    logger -s -p user.warn -t check_2nd_src "i2c dev doesn't exist on $FRU"
    echo $UNKNOWN_SRC
  fi
}

RET=$(check_fru_content)
printf "Source checking by FRU: %s\n" "$RET"

if [ "$RET" = "$UNKNOWN_SRC" ]; then
  RET=$(check_i2c_dev)
  printf "Source checking by i2c dev: %s\n" "$RET"
fi

logger -s -p user.warn -t check_2nd_src "$FRU source: $RET"

$KV_CMD set $KEY_SOURCE_INFO "$RET"

if [ -f /etc/modprobe.d/modprobe.conf ]; then
  rm /etc/modprobe.d/modprobe.conf
fi

if [ "$FRU" = "dpb" ]; then
  if [ ! -d /sys/bus/i2c/drivers/adc128d818 ] && [ "$RET" = "$MAIN_SRC" ]; then
    modprobe adc128d818
  fi
  if [ ! -d /sys/bus/i2c/drivers/ltc2991 ] && [ "$RET" = "$SECOND_SRC" ]; then
    modprobe ltc2991
  fi
fi

if [ "$FRU" = "e1s_iocm" ]; then
  if [ ! -d /sys/bus/i2c/drivers/ads1015 ] && [ "$RET" = "$MAIN_SRC" ]; then
    modprobe ti_ads1015
  fi
  if [ ! -d /sys/bus/i2c/drivers/ltc2990 ] && [ "$RET" = "$SECOND_SRC" ]; then
    modprobe ltc2990
  fi
fi
