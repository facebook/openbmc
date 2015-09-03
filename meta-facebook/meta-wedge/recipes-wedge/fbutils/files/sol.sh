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
#

CONSOLE_SH=/usr/local/bin/us_console.sh
FILE=/etc/us_pseudo_tty
TTY=/dev/ttyS1

if [ -a $FILE ]
  then
    read -r TTY<$FILE
fi

$CONSOLE_SH connect

echo "You are in SOL session."
echo "Use ctrl-x to quit."
echo "-----------------------"
echo

trap '"$CONSOLE_SH" disconnect' INT TERM QUIT EXIT

/usr/bin/microcom -s 57600 $TTY

echo
echo
echo "-----------------------"
echo "Exit from SOL session."
