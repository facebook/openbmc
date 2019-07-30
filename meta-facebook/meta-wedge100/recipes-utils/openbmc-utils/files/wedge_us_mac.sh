#!/bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
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

# SAMPLE_SIZE reached by testing in several wedge100 with noisy mac address results
SAMPLE_SIZE=18

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh

if ! wedge_is_us_on; then
    echo "Cannot retrive microserver MAC when microserver is powered off." 1>&2
    exit -2
fi

# Workaround wedge100 noisy mac address hw bug (in which some mac octets sometimes
# are 'ff') by querying for the mac address several times and getting the min()
# of each octet
sample="$(for i in $(seq 1 ${SAMPLE_SIZE}); do
  cat /sys/class/i2c-adapter/i2c-4/4-0033/mac
done)"

mac=""
for i in {1..6}; do
  min_octet=$(echo "$sample" | cut -f$i -d':' | sort | head -n1)
  mac+=":$min_octet"
done
mac="${mac:1}" # strip leading ':'

if [ -n "$mac" -a "${mac/X/}" = "${mac}" ]; then
    echo $mac
    exit 0
else
    echo "Cannot find out the microserver MAC" 1>&2
    exit -1
fi
