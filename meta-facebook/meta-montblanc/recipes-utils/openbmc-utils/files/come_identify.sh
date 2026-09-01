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
# Logic: Based on return code of `netlake_identify`:
#   - Return 1: Netlake 2.0 -> Setup SSIF-BMC device and module.
#   - Return 0: Netlake 1.0 -> Setup IPMB slave-mqueue device.
#   - Otherwise: Unknown version -> Output error and exit with status 1.
# ==============================================================================

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# Netlake 2.0 (SSIF) configuration
SSIF_I2C_BUS="${SSIF_I2C_BUS:-5}"
SSIF_I2C_ADDR="${SSIF_I2C_ADDR:-0x10}"

# Netlake 1.0 (IPMB / slave-mqueue) configuration
# Note: In 0x1010, the upper byte 0x1000 is the slave-mqueue flag,
# and the lower byte 0x10 is the 7-bit slave address.
IPMB_I2C_BUS="${IPMB_I2C_BUS:-5}"
IPMB_I2C_ADDR="${IPMB_I2C_ADDR:-0x1010}"

netlake_identify
netlake_type=$?
if [ "$netlake_type" -eq 1 ]; then
    # Netlake 2.0: Setup SSIF-BMC device and start ssifd service
    echo "Loading ssif modules for Netlake 2.0 on bus ${SSIF_I2C_BUS} addr ${SSIF_I2C_ADDR}"
    modprobe ssif_bmc
    i2c_device_add "${SSIF_I2C_BUS}" "${SSIF_I2C_ADDR}" ssif-bmc
    systemctl start ssifd.service
elif [ "$netlake_type" -eq 0 ]; then
    # Netlake 1.0: Setup IPMB slave-mqueue device and start ipmbd target
    echo "Loading ipmb modules for Netlake 1.0 on bus ${IPMB_I2C_BUS} addr ${IPMB_I2C_ADDR}"
    i2c_device_add "${IPMB_I2C_BUS}" "${IPMB_I2C_ADDR}" slave-mqueue
    systemctl start ipmbd.target
else
    echo "Error: Failed to identify Netlake version. Exit code: $netlake_type" >&2
    exit 1
fi

