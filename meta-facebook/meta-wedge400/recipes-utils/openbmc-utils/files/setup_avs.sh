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

VID_START=1.6 # Unit Voltage
VID_STEP=0.00625 # Unit Voltage
ISL_VOUT_PMBUS_COMMAND=0x21

TH3_ROV=$(i2cget -f -y 12 0x3e 0x46) # Get TH3 AVS value from SYSCPLD

if [[ $TH3_ROV -lt 0x72 || $TH3_ROV -gt 0x8A ]]; then 
    echo "Invalid AVS value (0x72 - 0x8A)."
    exit -1
fi

TH3_ROV_DEC=$(printf "%d" $TH3_ROV) 

VRM_VCC_MAX=$(echo $VID_START $TH3_ROV_DEC $VID_STEP|awk '{printf "%.5f", $1-(($2-2)*$3)}')

ISL_VOUT_mV=$(echo $VRM_VCC_MAX|awk '{printf "%d", $1*1000}') # Convert to mV

ISL_VOUT_PMBUS_VALUE=$(printf "0x%04x" $ISL_VOUT_mV) # Convert to HEX

i2cset -f -y 1 0x60 $ISL_VOUT_PMBUS_COMMAND $ISL_VOUT_PMBUS_VALUE w # Set VOUT to ISL68317

if [ $? -ne 0 ]; then 
    echo "Cannot communicate with ISL68317."
    exit -1
fi

ISL_VOUT_READBACK=$(i2cget -f -y 1 0x60 0x21 w | awk '{printf "%d", $1}') # Readback ISL68317 VOUT

if [ $ISL_VOUT_READBACK  -ne $ISL_VOUT_mV  ]; then
    echo "Failed to write the value to ISL68317."
    exit -1
fi
