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

#setup BMC SKU ID
gpio_export SKU_ID1_PAX0 GPIOB6
gpio_set SKU_ID1_PAX0 0

gpio_export SKU_ID0_PAX0 GPIOAC0
gpio_set SKU_ID0_PAX0 0

gpio_export SKU_ID1_PAX1 GPIOB7
gpio_set SKU_ID1_PAX1 0

gpio_export SKU_ID0_PAX1 GPIOAC2
gpio_set SKU_ID0_PAX1 1

gpio_export SKU_ID1_PAX2 GPIOM0
gpio_set SKU_ID1_PAX2 1

gpio_export SKU_ID0_PAX2 GPIOB0
gpio_set SKU_ID0_PAX2 0

gpio_export SKU_ID1_PAX3 GPIOM6
gpio_set SKU_ID1_PAX3 1

gpio_export SKU_ID0_PAX3 GPIOB4
gpio_set SKU_ID0_PAX3 1

# System power good
gpio_export SYS_PWR_READY GPIOB3

# PDB 12V POWER (output)
gpio_export BMC_IPMI_PWR_ON GPIOE1
gpio_set BMC_IPMI_PWR_ON 1