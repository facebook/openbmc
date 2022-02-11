#!/bin/bash
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

cmd_bic_ver="bic-util scm 0xe2 0x0b 0x15 0xa0 0x00 0x02"
COUNTER=0
MAX_RETRY=10
rex='^[0-9]+$'

echo "Initializing Bridge-IC I2C Frequency..."
while [ $COUNTER -lt $MAX_RETRY ]; do
    readarray -td ' ' bic_data <<<"$($cmd_bic_ver)"
    if [[ ${bic_data[4]} =~ $rex ]] ; then
        break
    fi
    sleep 3
    ((COUNTER+=1))
done

if [ $COUNTER -eq $MAX_RETRY ];then
    echo "Error: Cannot access Bridge-IC." >&2; exit 1
fi

bic_major=$(( 10#${bic_data[3]} ))
bic_minor=$(( 10#${bic_data[4]} ))
bic_ver="$bic_major.${bic_data[4]}"

echo Bridge-IC version is v"$bic_ver".

if [ "$bic_major" -ge 1 ];then
    if [ "$bic_minor" -ge 11 ];then
        ret=$(bic-util scm 0xe0 0x2a 0x15 0xa0 0x00 0x01)
        IANA="15 A0 00"
        if [ -z "${ret##*$IANA*}" ];then
                echo Bridge-IC is initialized SCL to 1Mhz.
        else
                echo "Error: Cannot access Bridge-IC." >&2; exit 1
        fi
    fi
fi

exit 0
