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
# Provides:          setup-pcie-config
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Setup PCIe Configuration
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

for i in 1 2 3 4
do
  if [[ $(get_server_type $i) == "2" && $(fby2_is_server_on $i) == "0" ]] ; then  # EP
    config=0
    if [ $(($i%2)) == 0 ] ; then
      slot_type=$(get_slot_type $(($i-1)))
      case "$slot_type" in
      "1")  # CF
        config=2
        ;;
      "2")  # GP
        config=1
        ;;
      *)
        config=0
        ;;
      esac
    fi

    retry=0
    while [ $retry -lt 3 ] ; do
      output=$(/usr/bin/bic-util slot$i 0xe0 0x31 0x15 0xa0 0x00 $config)
      if [ $? == 0 ] ; then
        break
      fi
      retry=$(($retry+1))
      usleep 100000
    done
  fi
done
