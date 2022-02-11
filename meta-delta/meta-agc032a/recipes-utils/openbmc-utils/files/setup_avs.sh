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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# Get AVS from SWPLD1, reg 0x2e, 0x2f
AVS_HIGH=$(i2cget -f -y 7 0x32 0x2e | awk '{printf "0x%04x",$1}')
AVS_LOW=$(i2cget -f -y 7 0x32 0x2f  | awk '{printf "0x%04x",$1}')
AVS=$(( $(($AVS_LOW << 0)) | $(($AVS_HIGH << 8)) ))
AVS_DATA=$(printf "0x%04x" $AVS)

if [[ $AVS_DATA -lt 0x0C29 || $AVS_DATA -gt 0x0E4E ]]; then
    echo "Invalid AVS value (0x0C29 - 0x0E4E)."
    exit -1
fi

IR_MUX_BUS=5
IR35233_ADDR=0x40
# PMBUS_PAGE=0x00
PMBUS_VOUT_MODE=0x20
PMBUS_VOUT_COMMAND=0x21
GET_VOUT_MODE=$(i2cget -f -y $IR_MUX_BUS $IR35233_ADDR $PMBUS_VOUT_MODE)
GET_VOUT_COMMAND=$(i2cget -f -y $IR_MUX_BUS $IR35233_ADDR $PMBUS_VOUT_COMMAND w)


# check vout_mode
if [[ $GET_VOUT_MODE -ne 0x14 ]]; then
    echo "Set PMBUS VOUT MODE = 0x14"
    i2cset -f -y $IR_MUX_BUS $IR35233_ADDR $PMBUS_VOUT_MODE 0x14
fi

# check vout_command
if [[ $GET_VOUT_COMMAND -ne $AVS_DATA ]]; then
    echo "Set PMBUS VOUT COMMAND = $AVS_DATA"
    i2cset -f -y $IR_MUX_BUS $IR35233_ADDR $PMBUS_VOUT_COMMAND $AVS_DATA w
fi


# VID_START=1.6 # Unit Voltage
# VID_STEP=0.00625 # Unit Voltage
# ISL_VOUT_PMBUS_COMMAND=0x21

# TH3_ROV=$(i2cget -f -y 12 0x3e 0x46) # Get TH3 AVS value from SYSCPLD

# if [[ $TH3_ROV -lt 0x72 || $TH3_ROV -gt 0x8A ]]; then
#     echo "Invalid AVS value (0x72 - 0x8A)."
#     exit -1
# fi

# TH3_ROV_DEC=$(printf "%d" $TH3_ROV)

# VRM_VCC_MAX=$(echo $VID_START $TH3_ROV_DEC $VID_STEP|awk '{printf "%.5f", $1-(($2-2)*$3)}')

# ISL_VOUT_mV=$(echo $VRM_VCC_MAX|awk '{printf "%d", $1*1000}') # Convert to mV

# ISL_VOUT_PMBUS_VALUE=$(printf "0x%04x" $ISL_VOUT_mV) # Convert to HEX

# i2cset -f -y 1 0x60 $ISL_VOUT_PMBUS_COMMAND $ISL_VOUT_PMBUS_VALUE w # Set VOUT to ISL68317

# if [ $? -ne 0 ]; then
#     echo "Cannot communicate with ISL68317."
#     exit -1
# fi

# ISL_VOUT_READBACK=$(i2cget -f -y 1 0x60 0x21 w | awk '{printf "%d", $1}') # Readback ISL68317 VOUT

# if [ $ISL_VOUT_READBACK  -ne $ISL_VOUT_mV  ]; then
#     echo "Failed to write the value to ISL68317."
#     exit -1
# fi
