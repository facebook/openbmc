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

### BEGIN INIT INFO
# Provides:          gpio-setup
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Set up GPIO pins as appropriate
### END INIT INFO

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

setup_gpio ABOOT_GRAB  GPIOB0 out 0

setup_gpio BMC_ALIVE  GPIOB1 out 1

setup_gpio CPU_MSMI_L  GPIOB2 in 

setup_gpio CP_PWRGD  GPIOB3 in 

setup_gpio SCM_OVER_TEMP  GPIOB4 in 

setup_gpio SCM_PWRGD  GPIOB6 in 

setup_gpio SMB_PWRGD  GPIOB7 in 

setup_gpio SW_JTAG_SEL  GPIOD2 out 0

setup_gpio SW_CPLD_JTAG_SEL  GPIOD3 out 0

setup_gpio JTAG_TRST_L  GPIOI0 out 0

setup_gpio CPU_CATERR_L  GPIOP1 in 

setup_gpio USB_DONGLE_PRSNT  GPIOP2 in 

setup_gpio CPLD_2_BMC_INT  GPIOS7 in 

setup_gpio SW_SPI_SEL  GPIOV0 out 0

setup_gpio CPU_RST_L  GPIOV1 in 

setup_gpio WDT1_RST  GPIOY0 out 0

setup_gpio SCM_TEMP_ALERT  GPIOY2 in 

setup_gpio BMC_EMMC_RST  GPIOY3 out 0

setup_gpio BMC_LITE_L  GPIOZ2 out 0