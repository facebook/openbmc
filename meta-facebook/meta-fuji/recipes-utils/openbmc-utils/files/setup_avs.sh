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

# Check board version. Only set avs VOUT from EVT3
# Since the TH4 AVS feature can't be enabled on EVT1,2 hardware units because of hardware factors. 
# But we found the SMB EVT1 board ID is incorrect, it's the same as DVT1 board ID, 
# and there are 3 EVT1 units(F301220170001, F301220170002, F301220240001) shipped to FB. 
# So it seems FB can't upgrade the 3 EVT1 units to next package.

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
#shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

board_ver=$(wedge_board_rev) #Get board version
# VDD AVS need to config for DVT1 and later
if [ "$board_ver" == "BOARD_FUJI_EVT1" ] || \
[ "$board_ver" == "BOARD_FUJI_EVT2" ] || \
[ "$board_ver" == "BOARD_FUJI_EVT3" ]; then
    echo "No need to set avs VOUT."
    exit 1
fi

#
# We have seen intermittent AVS voltage reading errors and root cause is
# not clear as of now. Testing shows below retry will help to mitigate
# such error.
#
fixup_avs_volt() {
    echo "Check AVS voltage..."

    for((i=1;i<=5;i++)); do
        # some delay to minimize driver binding error potentially caused by
        # dual-core or software race conditions. root cause not clear yet.
        sleep 1

        AVS_VOLT=$(cat "${SYSFS_I2C_DEVICES}"/1-0040/hwmon/hwmon*/in3_input)
        ret=$?
        #
        # Note: below range (500, 1200) millivolts is confirmed by hardware
        # team; please reach out to hardware team before making changes.
        #
        if [ $((ret)) -eq 0 ] && [ $((AVS_VOLT)) -gt 500 ] && [ $((AVS_VOLT)) -lt 1200 ]; then
            echo "AVS-Volt is normal, no fixup needed."
            return
        else
            echo "Invalid AVS-Volt $AVS_VOLT, re-create xdpe132g5c $i times"

            i2c_device_delete 1 0x40
            i2c_device_add 1 0x40 xdpe132g5c
        fi
    done

    logger -p user.err "Unable to fix AVS voltage after 5 retries"
}

fixup_avs_volt

VID_VCC_MAX=(0xE6B 0xE0C 0xDB2 0xD58 0xCF6)
XDPE_VOUT_PMBUS_COMMAND=0x21

TH4_ROV=$(i2cget -f -y 12 0x3e 0x56 | awk '{printf "%d", $1}') # Get TH4 AVS value from SYSCPLD

case $TH4_ROV in
    126|130|134|138|142) #0x7e,0x82,0x86,0x8a,0x8e
        ;;
    *)
        echo "Invalid AVS value[126,130,134,138,142]"
        exit 1
        ;;
esac

TH4_INDEX=$(((TH4_ROV - 126) / 4))
# Set VOUT to XDPE132G5C
i2cset -f -y 1 0x40 0x00 0xff # Allows access all pages 
XDPE_VDD_MAX_VALUE=$((VID_VCC_MAX[TH4_INDEX]))
XDPE_VDD_MAX_VALUE_HEX=$(printf "0x%04x" $XDPE_VDD_MAX_VALUE) # Convert to HEX

if ! i2cset -f -y 1 0x40 $XDPE_VOUT_PMBUS_COMMAND "$XDPE_VDD_MAX_VALUE_HEX" w; then 
    echo "Cannot communicate with XDPE132G5C."
    exit 1
fi
# Change XDPE132G5C from all pages to page 0.
i2cset -y -f 1 0x40 0x0 0x0
echo "Setup AVS Success"

