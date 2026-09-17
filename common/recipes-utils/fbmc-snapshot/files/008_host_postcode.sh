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

dump_log() {
	echo -e "\n#### $2 ####"
	if [ ! -f "$1" ]; then
		echo "$1 doesn't exist!"
	else
		cat "$1"
	fi
}

echo -e "\n############################################"
echo "########## HOST (uServer) CPU LOGS ##########"
echo "############################################"

dump_log /var/log/postcode_current "HOST POST CODE Current"
dump_log /mnt/data/postcode_last "HOST POST CODE Last"
