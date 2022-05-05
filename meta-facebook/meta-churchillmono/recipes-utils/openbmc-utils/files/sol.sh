#!/bin/sh
#
# Copyright (c) 2022 Cisco Systems Inc.
# Copyright (c) Meta Platforms, Inc. and affiliates.
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

#shellcheck disable=SC1091,SC2009
. /usr/local/bin/openbmc-utils.sh

CONSOLE_SH=/usr/local/bin/us_console.sh
FILE=/etc/us_pseudo_tty
TTY=/dev/ttyS4

mTerm_server_running() {
  pid=$(ps | grep -v grep | grep '/usr/local/bin/mTerm_server' -m 1 |
        awk '{print $1}')
  if [ "$pid" ] ; then
    return 0
  fi
  return 1
}

start_sol_session() {
  if [ -e $FILE ]; then
    read -r TTY<$FILE
  fi
  $CONSOLE_SH connect

  echo "You are in SOL session."
  echo "Use ctrl-x to quit."
  echo "-----------------------"
  echo

  trap '"$CONSOLE_SH" disconnect' INT TERM QUIT EXIT

  /usr/bin/microcom -s 57600 "$TTY"

  echo
  echo
  echo "-----------------------"
  echo "Exit from SOL session."
}

# if mTerm server is running use mTerm_client to connect to userver
# otherwise fallback to the old method
echo 0x3 > "$SMBCPLD_SYSFS_DIR/uart_selection"
if mTerm_server_running; then
  exec /usr/local/bin/mTerm_client wedge
else
  start_sol_session
fi
