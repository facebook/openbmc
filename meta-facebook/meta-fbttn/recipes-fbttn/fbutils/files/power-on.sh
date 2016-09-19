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
TO_PWR_ON=

check_por_config()
{

  TO_PWR_ON=-1

  # Check if the file/key doesn't exist
  if [ ! -f "${KEYDIR}/slot${1}_por_cfg" ]; then
    TO_PWR_ON=$DEF_PWR_ON
  else
    POR=`cat ${KEYDIR}/slot${1}_por_cfg`

    # Case ON
    if [ $POR == "on" ]; then
      TO_PWR_ON=1;

    # Case OFF
    elif [ $POR == "off" ]; then
      TO_PWR_ON=0;

    # Case LPS
    elif [ $POR == "lps" ]; then

      # Check if the file/key doesn't exist
      if [ ! -f "${KEYDIR}/pwr_server${1}_last_state" ]; then
        TO_PWR_ON=$DEF_PWR_ON
      else
        LS=`cat ${KEYDIR}/pwr_server${1}_last_state`
        if [ $LS == "on" ]; then
          TO_PWR_ON=1;
        elif [ $LS == "off" ]; then
          TO_PWR_ON=0;
        fi
      fi
    fi
  fi
}

# Sync BMC's date with one of the four servers
sync_date()
{
  for i in 1
  do
    if [ $(is_server_prsnt $i) == "1" ] ; then
      # Use standard IPMI command 'get-sel-time' to read RTC time
      output=$(/usr/local/bin/me-util slot$i 0x28 0x48)
      # if the command fails, continue to next slot
      [ $(echo $output | wc -c) != 12 ] && continue
      col1=$(echo $output | cut -d' ' -f1 | sed 's/^0*//')
      #col2=$(echo $output | cut -d' ' -f2 | sed 's/^0*//')
      #col3=$(echo $output | cut -d' ' -f3 | sed 's/^0*//')
      #col4=$(echo $output | cut -d' ' -f4 | sed 's/^0*//')

      # create the integer from the hex bytes returned
      val=$((0x$col4 << 24 | 0x$col3 << 16 | 0x$col2 << 8 | 0x$col1))

      # create the timestamp required for busybox's date command
      ts=$(date -d @$val +"%Y.%m.%d-%H:%M:%S")

      # set the command
      echo Syncing up BMC time with server$i...
      date $ts
      break
    fi
  done
}

# Check whether it is fresh power on reset
if [ $(is_bmc_por) -eq 1 ]; then

  # Disable clearing of PWM block on WDT SoC Reset
  devmem_clear_bit $(scu_addr 9c) 17

  sync_date

  # Get TCA9555 IO-EXP status i2c bus 5 0x24 (7bit)
  #P14 SCC_A_STBY_PGOOD
  #P15 SCC_A_FULL_PGOOD
  # To read TCA955 port 2

  # TODO: check sku (type 5/7)
  # TODO: SCC STBY POWER Enable

  # Read SCC STBY POWER good
  pwr_sequence_status=0
  pca9555=`i2cget -y 5 0x24 1`
  if [ $? -eq 0 ]; then
         pca9555=$[16#`echo "${pca9555}" | cut -d 'x' -f2`]

         # TODO: Check if power good
         scc_stby_good=$[$((${pca9555}))>>4]
         scc_stby_good=$[${scc_stby_good}&1]
  else
         # TODO: Record the error message
         pwr_sequence_status=$(($pwr_sequence_status+1))
         echo "I2C transaction failed."
  fi

  if [ $pwr_sequence_status -eq 0 ]; then
         pca9555=`i2cget -y 5 0x24 1`
         if [ $? -eq 0 ]; then
                # Read SCC FULL POWER good
                pca9555=$[16#`echo "${pca9555}" | cut -d 'x' -f2`]

                # TODO: Check if power good
                scc_full_good=$[$((${pca9555}))>>5]
                scc_full_good=$[${scc_full_good}&1]
         else
                # TODO: Record the error message
                echo "I2C transaction failed."
         fi
  fi

  # For Triton MonoLake PWR sequence
  if [ $(gpio_get $IOM_FULL_GOOD) = 0 ]; then
  #set ML board power en
  gpio_set O7 1

          check_por_config 1
          if [ $TO_PWR_ON -eq 1 ] && [ $(is_server_prsnt 1) == "1" ] ; then
                power-util slot1 on
          fi
  else
        echo "IOM PWR Fail"
  fi

  check_por_config 1
  if [ $TO_PWR_ON -eq 1 ] && [ $(is_server_prsnt 1) == "1" ] ; then
    power-util slot1 on
  fi
fi
