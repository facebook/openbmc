#!/bin/bash
#
# Copyright 2025-present Facebook. All Rights Reserved.
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

start_sol_session() {
  echo "------------------TERMINAL MULTIPLEXER---------------------"
  echo "Display help message: Not supported inside the session"
  echo "Default escape sequence: <CTRL-l x>"
  echo "/var/log/obmc-console-host0.log : Log location"
  echo "Send Break: <CTRL-l b> "
  echo "-----------------------------------------------------------"
  echo
  # Start obmc-console session with the escape sequence as 'CTRL-l x'
  obmc-console-client -c /etc/obmc-console/client.conf -i host0

  echo
  echo "Connection closed."
}

start_sol_session
