#!/bin/bash
#
# Copyright 2026-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program in a file named COPYING; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

# Read AST2750 register SCU000 [23:16] to retrieve the board hardware revision.

SCU000_REG=0x12c02000
HW_REV_BIT=16
BOARD_REV=$(($(($(devmem "${SCU000_REG}") >> "${HW_REV_BIT}")) & 0xff))
echo "BMC Board Revision: A${BOARD_REV}"
