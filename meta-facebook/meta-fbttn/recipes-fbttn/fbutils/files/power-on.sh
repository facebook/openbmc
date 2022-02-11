#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
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
# Provides:          power-on
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Power on Server
### END INIT INFO
. /usr/local/fbpackages/utils/ast-functions

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

KEYDIR=/mnt/data/kv_store
DEF_PWR_ON=1
TO_PWR_ON=-1

# Check power policy
# $1: slot number
check_por_config()
{
  TO_PWR_ON=-1
  slot_num=$1

  # Check if the file/key doesn't exist
  if [ ! -f "${KEYDIR}/slot${slot_num}_por_cfg" ]; then
    TO_PWR_ON=$DEF_PWR_ON
  else
    PWR_POLICY=`cat ${KEYDIR}/slot${slot_num}_por_cfg`

    # Case ON
    if [ "$PWR_POLICY" == "on" ]; then
      TO_PWR_ON=1;

    # Case OFF
    elif [ "$PWR_POLICY" == "off" ]; then
      TO_PWR_ON=0;

    # Case LPS
    elif [ "$PWR_POLICY" == "lps" ]; then

      # Check if the file/key doesn't exist
      if [ ! -f "${KEYDIR}/pwr_server${slot_num}_last_state" ]; then
        TO_PWR_ON=$DEF_PWR_ON
      else
        LS=`cat ${KEYDIR}/pwr_server${slot_num}_last_state`
        if [ "$LS" == "on" ]; then
          TO_PWR_ON=1;
        elif [ "$LS" == "off" ]; then
          TO_PWR_ON=0;
        fi
      fi
    fi
    logger -s -p user.info -t power-on "Power Policy: $PWR_POLICY, Last Power State: $LS"
  fi
}

# Check if BIC is ready or not
check_bic_ready()
{
  retry=0
  while [ "$retry" != "5" ]
  do
    if [ "$(gpio_get BIC_READY_N)" == "0" ]; then
      logger -s -p user.warn -t power-on "BIC is Ready!"
      return
    else
      logger -s -p user.info -t power-on "BIC is NOT Ready!!! Retry: $retry"
      retry=$((retry+1))
      sleep 1
    fi
  done
}

# Sync BMC's date with one of the four servers
sync_date()
{
  retry=0
  if [ "$(is_server_prsnt)" == "1" ] ; then
    while [ "$retry" != "5" ]
    do
      # Use standard IPMI command 'get-sel-time' to read RTC time
      output=$(/usr/local/bin/me-util server 0x28 0x48)
      # Sync BMC time only when ME returns correct response length
      if [ "$(echo $output | wc -c)" == "12" ] ; then
        col1=$(echo $output | cut -d' ' -f1 | sed 's/^0*//')
        col2=$(echo $output | cut -d' ' -f2 | sed 's/^0*//')
        col3=$(echo $output | cut -d' ' -f3 | sed 's/^0*//')
        col4=$(echo $output | cut -d' ' -f4 | sed 's/^0*//')
        # create the integer from the hex bytes returned
        val=$((0x$col4 << 24 | 0x$col3 << 16 | 0x$col2 << 8 | 0x$col1))
        # create the timestamp required for busybox's date command
        ts=$(date -d @$val +"%Y.%m.%d-%H:%M:%S")
        # set the command
        logger -s -p user.info -t power-on "Syncing up BMC time with server..."
        logger -s -p user.info -t power-on "`date $ts`"
        return
      else
        retry=$((retry+1))
        logger -s -p user.info -t power-on "Get SEL time from ME failed, syncing up BMC time with server failed for $retry times"
        sleep 1
      fi
    done
  fi
}

is_server_12v_off="1"

# Check Mono Lake is  present or not:
# If Mono Lake absent, turn off Mono Lake HSC 12V and IOM 3V3
is_server_12v_off="1"
if [ "$(is_server_prsnt)" == "0" ]; then
  logger -s -p user.warn -t power-on "The Mono Lake is absent, turn off Mono Lake HSC 12V and IOM 3V3."
  gpio_set COMP_PWR_EN 0
  gpio_set IOM_FULL_PWR_EN 0
  is_server_12v_off="0"
fi

# Check whether it is fresh power on reset
if [ "$(is_bmc_por)" == "1" ] && [ "$is_server_12v_off" == "1" ]; then
  logger -s -p user.warn -t power-on "BMC POR and Server is Present"
  # Disable clearing of PWM block on WDT SoC Reset
  devmem_clear_bit $(scu_addr 9c) 17

  # For fbttn remote SCC PWR sequence
  sh /usr/local/bin/check_pal_sku.sh > /dev/NULL
  PAL_SKU=$(($(($? >> 6)) & 0x1))
  if [ "$PAL_SKU" == "1" ]; then  # type 7, always power-on SCC B
    gpio_set SCC_RMT_FULL_PWR_EN 1
    gpio_tolerance_fun SCC_RMT_FULL_PWR_EN GPIOF1
  fi

  # Turn on the IOM_FULL_PWR_EN to power on the M.2 and IOM 3v3 
  gpio_set IOM_FULL_PWR_EN 1
  logger -s -p user.warn -t power-on "Turning on IOM 3v3(IOM_FULL_PWR_EN)"
  # For fbttn MonoLake PWR sequence
  if [ "$(gpio_get IOM_FULL_PGOOD)" == "1" ]; then
    logger -s -p user.warn -t power-on "BMC full power good is enable"
    # set ML board power enable (12V on)
    gpio_set COMP_PWR_EN 1
    logger -s -p user.warn -t power-on "Turning Server 12V-on"
    sleep 3  # waiting for ME ready
    check_bic_ready    
    sync_date
    check_por_config 1
    if [ "$TO_PWR_ON" == "1" ] && [ "$(is_server_prsnt)" == "1" ] ; then
        power-util server on
    fi
  else
    logger -s -p user.warn -t power-on "IOM full power good is disable"
  fi
else
  logger -s -p user.warn -t power-on "BMC Rebooted or Server is not Present"
  sync_date
fi
