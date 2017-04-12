#!/bin/sh
#
# Copyright 2016-present Facebook. All Rights Reserved.
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

### BEGIN INIT INFO
# Provides:          setup-confg
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: setup configure files.
# The file format should be src:dest
### END INIT INFO

ACTION="$1"
case "$ACTION" in
  start)
  if [ -e /etc/config ] && [ "$(ls -A /etc/config)" ]; then
    echo "Start to copy configure files...."
    for file in /etc/config/* 
    do
       while IFS='' read -r line || [[ -n "$line" ]]; do
         src=$(echo $line | awk -F":" '{print $1}')
         dest=$(echo $line | awk -F":" '{print $2}')
         if [ -e $src ]; then
	         mkdir -p $dest
           cp -r $src $dest
         fi
       done < "$file"
    done
    echo "Copying is done."
  fi
esac

exit 0
