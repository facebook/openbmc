#!/bin/bash
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

# ==============================================================================
# TODO / STUB: Netlake Version Identification
#
# Hardware Spec Reference:
#   Identify NL1.0 or NL2.0 through COMe FPGA reg.
#   COMe FPGA Bus:0 SLA:0x1F reg:0x01
#   Description: Bit7: System_NL2_Flag (0=NL1.0, 1=NL2.0)
#
# Version-specific module loading logic
# NOTE: Operations under NL1.0 and NL2.0 are currently bypassed. 
#       The logic structure is kept intact for future extension.
# ==============================================================================

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

netlake_identify
netlake_type=$?
if [ "$netlake_type" -eq 1 ]; then
    # TODO: Add NL2.0 specific module loading/operations here if needed in the future
    :
elif [ "$netlake_type" -eq 0 ]; then
    # TODO: Add NL1.0 specific module loading/operations here if needed in the future
    :
else
    echo "Error: Failed to identify Netlake version. Exit code: $netlake_type" >&2
    exit 1
fi

