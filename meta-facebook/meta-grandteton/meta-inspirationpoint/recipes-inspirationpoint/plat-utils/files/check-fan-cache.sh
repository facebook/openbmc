#!/bin/bash
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
#

. /usr/local/fbpackages/utils/ast-functions

HGX_ERR_READ_FAIL=1
HGX_ERR_UNSUPPORT=2

KV_CMD="/usr/bin/kv"
FAN_TABLE_VER_500W="500"
FAN_TABLE_VER_650W="650"
DEFAULT_FSC_CONFIG="/etc/fsc-config.json"

hgx_pwr_limit_check () {
  curr_fan_table=`$KV_CMD get "auto_fsc_config" persistent`

  recheck_500w=0
  recheck_650w=0
  for ((loop=0; loop<2; loop++)); do
    pwr_limit=`/usr/local/bin/hgxmgr get-pwr-limit | awk '{print $NF}'`
    count_500W=$(echo "$pwr_limit" | grep -o "500W" | wc -l)
    count_650W=$(echo "$pwr_limit" | grep -o "650W" | wc -l)

    # Return an error when fail to get GPU power limit
    if [ -z "$pwr_limit" ]; then
      exit $HGX_ERR_READ_FAIL
    fi

    # Only two configuration are supported: 500W and 650W.
    # Return an error when the current setting is not supported.
    if [ $count_500W -eq 0 ] && [ $count_650W -eq 0 ]; then
      exit $HGX_ERR_UNSUPPORT
    fi

    if [ "$count_500W" -ge "$count_650W" ] &&
       [ "$curr_fan_table" == "$FAN_TABLE_VER_650W" ]; then
      recheck_500w=$((recheck_500w+1))

      if [ $recheck_500w -eq 2 ]; then
        $KV_CMD set "auto_fsc_config" 500 persistent
        rm -f ${DEFAULT_FSC_CONFIG}
        ln -s /etc/fsc-config-8-retimer-500W.json ${DEFAULT_FSC_CONFIG}
        logger -t "fscd" -p daemon.crit "Fan table is changing from 650W to 500W"
        sv restart fscd
        recheck_500w=0
      fi
    elif [ "$count_500W" -lt "$count_650W" ] &&
         [ "$curr_fan_table" == "$FAN_TABLE_VER_500W" ]; then
      recheck_650w=$((recheck_650w+1))

      if [ $recheck_650w -eq 2 ]; then
        $KV_CMD set "auto_fsc_config" 650 persistent
        rm -f ${DEFAULT_FSC_CONFIG}
        ln -s /etc/fsc-config-8-retimer-650W.json ${DEFAULT_FSC_CONFIG}
        logger -t "fscd" -p daemon.crit "Fan table is changing from 500W to 650W"
        sv restart fscd
        recheck_650w=0
      fi
    fi

    if [ $recheck_500w -eq 1 ] || [ $recheck_650w -eq 1 ]; then
      sleep 30
    else
      break
    fi
  done
}

is_fscd_running=`ps | grep fscd | grep -v grep | grep "runsv /etc/sv/fscd"`
auto_fsc=$1

if [ -n "$is_fscd_running" ] && [ "$auto_fsc" == "hgx" ]; then
  hgx_pwr_limit_check
  exit 0
fi

check_mb_rev() {
  rev_id=$(kv get mb_rev)
  if [[ "$rev_id" == "2" ]]; then
    ln -s /etc/fsc-config-2-retimer.json ${DEFAULT_FSC_CONFIG}
  else
    curr_fan_table=`$KV_CMD get "auto_fsc_config" persistent`

    # The default fan table is configued as 500W
    if [ -z "$curr_fan_table" ]; then
      $KV_CMD set "auto_fsc_config" 500 persistent
    else
      rm -f ${DEFAULT_FSC_CONFIG}
    fi

    if [ "$curr_fan_table" == "$FAN_TABLE_VER_500W" ]; then
      ln -s /etc/fsc-config-8-retimer-500W.json ${DEFAULT_FSC_CONFIG}
    elif [ "$curr_fan_table" == "$FAN_TABLE_VER_650W" ]; then
      ln -s /etc/fsc-config-8-retimer-650W.json ${DEFAULT_FSC_CONFIG}
    fi
  fi
}

for retry in {1..20};
do
    bp2_sensor238=$(kv get fan_bp2_sensor238)
    if [ $? -ne 0 ]; then
        sleep 3
    else
        break
    fi
done

#check M1 test board
tb=$(kv get m1_test_board)

if [ "$tb" -eq 0 ]; then
    val=$(i2cget -f -y 40 0x21 0x00)
    fan_present1=$(("$val"))

    val=$(i2cget -f -y 41 0x21 0x00)
    fan_present2=$(("$val"))

    if [ "$fan_present1" -eq 255 ] && [ "$fan_present2" -eq 255 ]; then
      echo "Don't enable fscd due to fan not present"
    else
      echo "Start fscd"
      check_mb_rev
      runsv /etc/sv/fscd > /dev/null 2>&1 &
    fi
fi
echo "done."
