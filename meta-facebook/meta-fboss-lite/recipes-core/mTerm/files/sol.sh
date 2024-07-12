#!/bin/sh
#
# Copyright 2020-present Facebook. All Rights Reserved.
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
mTerm_server_running() {
  # shellcheck disable=SC2009
  pid=$(ps | grep -v grep | grep '/usr/local/bin/mTerm_server' -m 1 |
      awk '{print $1}')
  if [ "$pid" ] ; then
    return 0
  fi
  return 1
}

start_sol_session() {
  echo "You are in SOL session."
  echo "Use ctrl-x to quit."
  echo "-----------------------"
  echo

  /usr/bin/microcom -s @BAUDRATE@ /dev/@TERM@

  echo
  echo
  echo "-----------------------"
  echo "Exit from SOL session."
}

handle_mterm_failure() {
  echo "Error: mTerm_server is Not running!"
  echo "If the system rebooted recently, please wait until your system has"
  echo "been up for at least 5 minutes."
  echo "If mTerm_server service is down unexpectedly, please reach out to"
  echo "@fboss_platform team for support."
  echo "Use <microcom -s @BAUDRATE@ /dev/@TERM@> with caution: an active microcom"
  echo "session could block console access from future sol.sh instances."
  echo "Exiting!"
  exit 1
}

#
# if mTerm server is running, use mTerm_client to connect to userver.
# otherwise, exit with errors instead of falling back to microcom.
#

if mTerm_server_running; then
  exec /usr/local/bin/mTerm_client wedge
else
  handle_mterm_failure
fi
