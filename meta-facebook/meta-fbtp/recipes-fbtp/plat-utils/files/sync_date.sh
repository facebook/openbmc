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

PATH=/sbin:/bin:/usr/sbin:/usr/bin

# Sync BMC's date with the server
sync_date()
{
  # Use standard IPMI command 'get-sel-time' to read RTC time
  output=$(/usr/local/bin/me-util 0x28 0x48)
  # if the command fails, return
  [ $(echo $output | wc -c) != 12 ] && return
  col1=$(echo $output | cut -d' ' -f1 | sed 's/^0*//')
  col2=$(echo $output | cut -d' ' -f2 | sed 's/^0*//')
  col3=$(echo $output | cut -d' ' -f3 | sed 's/^0*//')
  col4=$(echo $output | cut -d' ' -f4 | sed 's/^0*//')

  # create the integer from the hex bytes returned
  val=$((0x$col4 << 24 | 0x$col3 << 16 | 0x$col2 << 8 | 0x$col1))

  # create the timestamp required for busybox's date command
  ts=$(date -d @$val +"%Y.%m.%d-%H:%M:%S")

  # set the command
  echo Syncing up BMC time with server...
  date $ts
}

sync_date

