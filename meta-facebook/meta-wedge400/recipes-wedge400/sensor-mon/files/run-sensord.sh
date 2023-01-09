#!/bin/sh
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

. /usr/local/bin/openbmc-utils.sh

ARGS="scm smb fan1 fan2 fan3 fan4"

#
# PSU and PEM will never be plugged to a switch at the same time, and
# they won't be hotswapped. So we only need to mointor the power units
# that are plugged at present.
#
# TODO: improve the logic to monitor only one PEM.
#
if [ "$(wedge_power_supply_type)" = "PEM" ]; then
    ARGS="$ARGS pem1 pem2"
else
    ARGS="$ARGS psu1 psu2"
fi

#shellcheck disable=SC2086
exec /usr/local/bin/sensord $ARGS
