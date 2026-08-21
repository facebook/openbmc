#!/bin/sh
#
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

# Identify NL1.0 or NL2.0 through COMe FPGA reg.
# COMe FPGA Bus:0 SLA:0x1F reg:0x01 Description:Bit7: System_NL2_Flag (0=NL1.0, 1=NL2.0)
#
#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# The default SSIF device is 5-0010.
# The platform-specific .bbappend can override this value through
# a systemd override.conf file by setting:
#   [Service]
#   Environment="SSIF_DEVICE=<bus>-<address>"
SSIF_DEVICE="${SSIF_DEVICE:-5-0010}"

netlake_identify
netlake_type=$?
if [ "$netlake_type" -eq 1 ]; then
    # Netlake 2.0
    exit 0

elif [ "$netlake_type" -eq 0 ]; then
    echo "Netlake 1.0 does not support SSIF, skip starting ssifd"
    echo "$SSIF_DEVICE" > /sys/bus/i2c/drivers/ipmi-ssif-host/unbind
    exit 1

else
    echo "Failed to identify Netlake version, skip starting ssifd"
    exit 1
fi
