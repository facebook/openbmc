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

#shellcheck disable=SC1091
PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
. /usr/local/bin/openbmc-utils.sh

# Skip AVS setup on Wedge400C unit
if [ "$(wedge_board_type)" != "0" ]; then
    exit 0
fi

VID_START=1.6 # Unit Voltage
VID_STEP=0.00625 # Unit Voltage
ISL_VOUT_PMBUS_COMMAND=0x21

VALID_MAX=0x8A
VALID_MIN=0x72

MAX_RETRY=5
power_count=0
avs_count=0
while true; do
    th3_power_on=$(head -n 1 "$SMBCPLD_SYSFS_DIR/th3_turn_on" 2> /dev/null)
    th3_power_good=$(head -n 1 "$SMBCPLD_SYSFS_DIR/vddcore_th3_power_good" 2> /dev/null)
    if [ "$((th3_power_good))" != "1" ] || [ "$((th3_power_on))" != "1" ]; then
        sleep 1
        power_count=$((power_count + 1))
        if [ $power_count -ge $MAX_RETRY ]; then
            if [ $power_count -eq $MAX_RETRY ]; then
                echo "$0 timeout, set AVS directly"
            fi
        else
            continue
        fi
    fi

    sleep 0.5

    TH3_ROV=$(i2cget -f -y 12 0x3e 0x46) # Get TH3 AVS value from SYSCPLD
    if [ $((TH3_ROV)) -lt $((VALID_MIN)) ] || [ $((TH3_ROV)) -gt $((VALID_MAX)) ]; then 
        avs_count=$((avs_count + 1))
        if [ $avs_count -ge $MAX_RETRY ]; then
            echo "Invalid AVS value: $TH3_ROV out of range: ($VALID_MIN - $VALID_MAX)"
            echo "Will not set AVS voltage, use ISL68317 default output voltage"
            exit 1
        else
            continue
        fi
    fi

    TH3_ROV_DEC=$(printf "%d" "$TH3_ROV") 

    VRM_VCC_MAX=$(echo $VID_START "$TH3_ROV_DEC" $VID_STEP | awk '{printf "%.5f", $1-(($2-2)*$3)}')

    ISL_VOUT_mV=$(echo "$VRM_VCC_MAX" | awk '{printf "%d", $1*1000}') # Convert to mV

    ISL_VOUT_PMBUS_VALUE=$(printf "0x%04x" "$ISL_VOUT_mV") # Convert to HEX

    # Set VOUT to ISL68317
    if [ "$(i2cset -f -y 1 0x60 $ISL_VOUT_PMBUS_COMMAND $((ISL_VOUT_PMBUS_VALUE)) w)" ]; then
        echo "Cannot communicate with ISL68317."
        continue
    fi

    ISL_VOUT_READBACK=$(i2cget -f -y 1 0x60 0x21 w | awk '{printf "%d", $1}') # Readback ISL68317 VOUT

    if [ $((ISL_VOUT_READBACK)) -ne $((ISL_VOUT_mV)) ]; then
        echo "Failed to write the value to ISL68317."
        continue
    fi
    echo "Set AVS voltage to $ISL_VOUT_mV mV, read back is $ISL_VOUT_READBACK mV"
    exit 0
done
