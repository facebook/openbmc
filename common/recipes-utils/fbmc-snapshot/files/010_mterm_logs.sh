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

# Collects the live log and any rotated copies.
echo -e "\n##### x86 mTerm Logs #####"
# /var/log is a symlink to /var/volatile/log, so both names reach the same file.
if [ ! -f /var/log/mTerm_wedge.log ]; then
	echo "/var/log/mTerm_wedge.log doesn't exist!"
else
	cat /var/log/mTerm_wedge.log
fi

echo -e "\n##### x86 mTerm Rotated Logs #####"
shopt -s nullglob
found=0
for log in /var/log/mTerm_wedge.log.*; do
	found=1
	echo -e "\n#### ${log} ####"
	cat "$log"
done
if [ "$found" -eq 0 ]; then
	echo "No rotated mTerm logs found."
fi
