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

((val=$(i2cget -f -y 12 0x31 0x3 2> /dev/null | head -n 1)))
if [ $val -ge 8 ]; then
	mv /etc/sensors.d/galaxy100_fab.conf /etc/sensors.d/galaxy100.conf
	rm /etc/sensors.d/galaxy100_lc.conf
else
	mv /etc/sensors.d/galaxy100_lc.conf /etc/sensors.d/galaxy100.conf
	rm /etc/sensors.d/galaxy100_fab.conf
fi
