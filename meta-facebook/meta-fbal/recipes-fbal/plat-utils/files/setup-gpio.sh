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

# FM_BMC_PWR_BTN_R_N, Server power button in, on GPIO E2(34)
# To enable GPIOE2, SCU80[18], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 18
devmem_clear_bit $(scu_addr 8C) 13
devmem_clear_bit $(scu_addr 70) 22

gpio_export FM_BMC_PWR_BTN_R_N GPIOE2

# FM_BMC_PWRBTN_OUT_R_N, power button to Server, on GPIO E3(35)
# To enable GPIOE3, SCU80[19], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 19
gpio_export FM_BMC_PWRBTN_OUT_R_N GPIOE3
gpio_set FM_BMC_PWRBTN_OUT_R_N 1

# RST_BMC_RSTBTN_OUT_R_N, power reset to Server, on GPIO E1(33)
devmem_clear_bit $(scu_addr 80) 17
gpio_export RST_BMC_RSTBTN_OUT_R_N GPIOE1
gpio_set RST_BMC_RSTBTN_OUT_R_N 1

# PWRGD_SYS_PWROK, power GOOD , on GPIO Y2(194)
# To enable GPIOY2, SCUA4[10] and SCU94[11] must be 0
devmem_clear_bit $(scu_addr A4) 10
devmem_clear_bit $(scu_addr 94) 11

gpio_export PWRGD_SYS_PWROK GPIOY2

# Power LED for Server:
gpio_export SERVER_POWER_LED GPIOE7  # Confirm Shadow name
gpio_set SERVER_POWER_LED 1

# FM_BMC_READY_N: GPIOG3 (51)
gpio_export FM_BMC_READY_N GPIOG3
gpio_set FM_BMC_READY_N 1

# FP_BMC_RST_BTN_N
devmem_clear_bit $(scu_addr 80) 16
gpio_export FP_BMC_RST_BTN_N GPIOE0
