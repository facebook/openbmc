#!/bin/bash
#
# Copyright 2017-present Facebook. All Rights Reserved.
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

#
# A simple script to toggle reset CP2112 device
#

CP2112_RESET_CMD='/sys/class/i2c-adapter/i2c-12/12-0031/cp2112_rst'

#
# Toggle reset CP2112, with enough time in-between for chip status settle down
#
echo 0x0 > $CP2112_RESET_CMD
usleep 50000
echo 0x1 > $CP2112_RESET_CMD
usleep 50000

#
# If the execution reached here, that means all writes went through
# return 0.
#
exit 0
