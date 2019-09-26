#!/bin/bash
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

### BEGIN INIT INFO
# Provides:          gpio-setup
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Set up GPIO pins as appropriate
### END INIT INFO

# This file contains definitions for the GPIO pins that were not otherwise
# defined in other files.  We should probably move some more of the
# definitions to this file at some point.

. /usr/local/fbpackages/utils/ast-functions

# FM_BMC_PWR_BTN_R_N
gpio_export FM_BMC_PWR_BTN_R_N GPIOE2

# FP_BMC_RST_BTN_N
gpio_export FP_BMC_RST_BTN_N GPIOE0

# RST_BMC_RSTBTN_OUT_R_N, power reset to PCH
gpio_export RST_BMC_RSTBTN_OUT_R_N GPIOE1
gpio_set RST_BMC_RSTBTN_OUT_R_N 1

# FM_BMC_PWRBTN_OUT_R_N, power button to CPLD
gpio_export FM_BMC_PWRBTN_OUT_R_N GPIOE3
gpio_set FM_BMC_PWRBTN_OUT_R_N 1

# PWRGD_SYS_PWROK, power GOOD from CPLD
gpio_export PWRGD_SYS_PWROK GPIOY2
