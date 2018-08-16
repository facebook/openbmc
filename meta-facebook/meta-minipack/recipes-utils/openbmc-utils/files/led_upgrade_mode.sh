#!/bin/bash
#
# Copyright 2018-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.

# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
source /usr/local/bin/openbmc-utils.sh

sys_led_alter=0
psu_led_alter=0
fan_led_alter=0
smb_led_alter=0

function set_led_color
{
  local component=$1 color=$2
  sh /usr/local/bin/set_sled.sh $component $color $board_rev
}

function led_alternate
{
  if [[ $1 -gt 0 ]]; then
    if [[ $2 = 0 ]]; then
      set_led_color $3 'blue'
      if [[ $3 = 'sys' ]];then
	    sys_led_alter=1
	  elif [[ $3 = 'psu' ]];then
	    psu_led_alter=1
	  elif [[ $3 = 'fan' ]];then
	    fan_led_alter=1
	  elif [[ $3 = 'smb' ]];then
	    smb_led_alter=1
      fi
    else
      set_led_color $3 'yellow'
      if [[ $3 = 'sys' ]];then
	    sys_led_alter=0
	  elif [[ $3 = 'psu' ]];then
	    psu_led_alter=0
	  elif [[ $3 = 'fan' ]];then
	    fan_led_alter=0
	  elif [[ $3 = 'smb' ]];then
	    smb_led_alter=0
      fi
    fi
  fi
}

while true; do
  sys_upgrade=$([[ -f /tmp/sys_upgrade_mode ]] && echo 1 || echo 0)
  psu_upgrade=$([[ -f /tmp/psu_upgrade_mode ]] && echo 1 || echo 0)
  fan_upgrade=$([[ -f /tmp/fan_upgrade_mode ]] && echo 1 || echo 0)
  smb_upgrade=$([[ -f /tmp/smb_upgrade_mode ]] && echo 1 || echo 0)
  if [[ $sys_upgrade -gt 0 ]] || [[ $psu_upgrade -gt 0 ]] || [[ $fan_upgrade -gt 0 ]] || [[ $smb_upgrade -gt 0 ]]; then
    led_alternate $sys_upgrade $sys_led_alter 'sys'
    led_alternate $psu_upgrade $psu_led_alter 'psu'
    led_alternate $fan_upgrade $fan_led_alter 'fan'
    led_alternate $smb_upgrade $smb_led_alter 'smb'
	sleep 1s
  else
    exit
  fi
done
