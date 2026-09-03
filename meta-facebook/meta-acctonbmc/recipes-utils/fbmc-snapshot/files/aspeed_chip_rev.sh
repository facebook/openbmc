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

# Read AST2620 register SCU014 [23:16] to retrieve the board hardware revision.

SCU014_REG=0x1e6e2014
HW_REV_BIT=16
BOARD_REV=$(($(($(devmem "${SCU014_REG}") >> "${HW_REV_BIT}")) & 0xff))
echo "ASPEED Chip Revision: A${BOARD_REV}"
