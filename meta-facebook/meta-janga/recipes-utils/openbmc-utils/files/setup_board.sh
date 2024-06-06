#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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
# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# set LPC signal strength pin to 0 (weakest)
setup_LPC_signal_strength 0

# set clock delay, MAC4 TX=9, MAC4 RX=5
ast2600_setup_RGMII34_clock_delay MAC4_TX=9 MAC4_RX=5
