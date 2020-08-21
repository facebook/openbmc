#!/bin/bash
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

### BEGIN INIT INFO
# Provides:          hclk_fixup
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Set up protype HCLK hacks
### END INIT INFO

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# units that require this workaround
matchRegex='JAS20240010|JAS20240013'

# get the MAC from Elbert SCM BMC EEPROM
if weutil bmc | grep -q -E "$matchRegex"; then
    # AST2620 SCU350 BIT31: RGMII select Internal PLL for MAC3/MAC4
    echo 'Running HCLK workaround for BMC...'
    devmem_set_bit $((0x1E6E2000 + 0x350)) 31
fi
