#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
# Provides:          setup-sensord
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Setup sensor monitoring
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup gpio monitoring for yosemite... "

# Check for the slots present and run sensord for those slots only.
SLOTS=
  if [ $(is_server_prsnt 1) == "1" ]; then
    SLOTS="$SLOTS slot1"
  fi

  if [ $(is_server_prsnt 2) == "1" ]; then
    SLOTS="$SLOTS slot2"
  fi

  if [ $(is_server_prsnt 3) == "1" ]; then
    SLOTS="$SLOTS slot3"
  fi

  if [ $(is_server_prsnt 4) == "1" ]; then
    SLOTS="$SLOTS slot4"
  fi

/usr/local/bin/gpiod $SLOTS

echo "done."
