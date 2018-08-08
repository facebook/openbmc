#!/bin/bash
#
# Copyright 2017-present Facebook. All Rights Reserved.
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

#
# A simple script to flush i2c clock signals to recover from I2C stuck issue.
#

#
# Initialize variables, and define a helper function
#

iteration=0
rc=0
I2C_CMD='/sys/class/i2c-adapter/i2c-12/12-0031/i2c_sta_rst_raw'

force_stop() {
	echo "Sending out START condition through I2C bus"
	echo 0x07 > $I2C_CMD
  usleep 5000
  echo 0x0F > $I2C_CMD
  usleep 5000
}

#
# Step 1. Hold both SDA and SCL high
#
echo "Holding SDA and SCL high"
echo 0x0F > $I2C_CMD

#
# Step 2. Toggle SCL up to 10 times, until bus recovers
#
while [ $iteration -lt 10 ];
do
	echo "Toggling SCL for clock flushing. Iteration : $iteration"
  # SCL low
	echo 0x0B > $I2C_CMD
  usleep 5000
  # SCL high
	echo 0x0F > $I2C_CMD
	usleep 5000
  # Check if bus recovered by looking at bit [5:4]
	rc=$(cat $I2C_CMD | head -n 1 | awk -F "0x" '{print $2}')
	if (($rc & 0x30)); then
		echo "Bus recovery successful"
		rc=0
		break
	fi
	echo "I2C BUS not yet recovered"
	iteration=$(($iteration+1))
done

#
# Step 3. Handle the result, and run force_stop if asked to do so
#         Setting return code to -1, if recovery is not successful
#
if [ $(($rc)) -ne 0 ]; then
	echo "Bus recovery failure"
  rc=-1
fi
if [ $# -eq 1 ] && [ $1 == "-f" ]; then
	force_stop
fi

#
# Step 4. Put the I2C block in normal operation mode
#
echo "Putting I2C master back to normal operation mode"
echo 0x0D > $I2C_CMD

#
# Step 5. Finally, toggle reset CP2112, no matter recovery successful or not
#
echo "Toggle resetting CP2112"
/bin/bash /usr/local/bin/reset_cp2112.sh

#
# Step 6. Exit with the return code
#
echo "Exiting with rc $rc"
exit $rc
