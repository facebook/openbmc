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

echo -e "\n##### BMC EEPROM INFO #####"

# elbert_weutil owns the device list, so the targets show_tech.py named one by
# one (CHASSIS SMB SCM PIM2-9 SMB_EXTRA BMC) are all covered here, in the same
# order. Each block is delimited by its own "Wedge EEPROM <device>:" line.
weutil -a
