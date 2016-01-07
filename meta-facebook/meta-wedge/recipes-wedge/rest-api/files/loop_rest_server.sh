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

# In v26 image : 1st release for fido , there were some python crashes
# observed from rest server. In order to test for any other issues in this build
# restart the rest server when it crashes.

CMD="/usr/local/bin/rest.py"
LOG="/tmp/rest_start.log"
count=0
while true; do
    pid=$(ps | grep -v grep | grep $CMD -m 1 | awk '{print $1}')
    if ! [ $pid ]; then
        echo "$(date) Starting RESTful server at $count times" > $LOG
        $CMD >> $LOG 2>&1
        (( count++ ))
    fi
    sleep 1
done
