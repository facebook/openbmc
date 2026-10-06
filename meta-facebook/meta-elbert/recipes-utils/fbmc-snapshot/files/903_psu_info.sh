#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

echo -e "\n################################"
echo "######## PSU DEBUG INFO ########"
echo "################################"

# PSU1-4 sit on SMBus 24-27.
bus=24
for psu in 1 2 3 4; do
	echo -e "\n##### PSU${psu} INFO #####"
	if [ ! -x /usr/local/bin/psu_show_tech.py ]; then
		echo "/usr/local/bin/psu_show_tech.py doesn't exist!"
		break
	fi
	/usr/local/bin/psu_show_tech.py "$bus" 0x58 -c generic
	bus=$((bus + 1))
done
