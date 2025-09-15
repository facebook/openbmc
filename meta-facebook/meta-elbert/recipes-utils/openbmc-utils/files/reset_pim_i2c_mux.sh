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

#
# The script resets the I2C MUX (2-0075) which connects BMC SoC and the
# I2C client devices on the 8 PIMs.
# It's used to recover i2c-2 when the bus is locked, usually caused by
# bad connectors on SCM (Supervisor), SMB (switch card) or PIMs.
#

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

PIM_I2C_MUX_BUS=2
PIM_I2C_MUX_ADDR=0x75
PIM_I2C_MUX_CHIP=pca9548
PIM_I2C_MUX_ID=$(printf "%d-%04x" "$PIM_I2C_MUX_BUS" "$PIM_I2C_MUX_ADDR")
PIM_I2C_MUX_NAME="SMB_PIM_I2C_MUX ($PIM_I2C_MUX_ID)"

echo "put ${PIM_I2C_MUX_NAME} in reset mode.."
echo 1 > "${SMBCPLD_SYSFS_DIR}/pim_smb_mux_rst"

sleep 1
echo "take ${PIM_I2C_MUX_NAME} out of reset.."
echo 0 > "${SMBCPLD_SYSFS_DIR}/pim_smb_mux_rst"

# Re-create i2c-mux device if no driver is attached.
PIM_MUX_SYSFS_DIR=$(i2c_device_sysfs_abspath "${PIM_I2C_MUX_ID}")
if [ ! -e "${PIM_MUX_SYSFS_DIR}/driver" ]; then
    echo "re-create ${PIM_I2C_MUX_NAME}.."
    i2c_device_delete "$PIM_I2C_MUX_BUS" "$PIM_I2C_MUX_ADDR"
    i2c_device_add "$PIM_I2C_MUX_BUS" "$PIM_I2C_MUX_ADDR" "$PIM_I2C_MUX_CHIP"
fi

# Let's see if the I2C mux is reachable
if i2cget -f -y "$PIM_I2C_MUX_BUS" "$PIM_I2C_MUX_ADDR" 0 > /dev/null 2>&1; then
    echo "${PIM_I2C_MUX_NAME} is reachable now!"
else
    echo "Error: ${PIM_I2C_MUX_NAME} is still down after reset!"
    exit 1
fi
