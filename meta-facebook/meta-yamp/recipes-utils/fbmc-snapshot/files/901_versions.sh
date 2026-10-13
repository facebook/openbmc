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

run_util() {
	echo -e "\n##### $2 #####"
	if [ ! -x "$1" ]; then
		echo "$1 doesn't exist!"
	else
		"$1"
	fi
}

run_util /usr/local/bin/fpga_ver.sh  "FPGA VERSIONS"
run_util /usr/local/bin/dpm_ver.sh   "DPM VERSIONS"
