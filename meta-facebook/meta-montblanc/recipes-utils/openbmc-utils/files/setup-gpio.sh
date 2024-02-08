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

# BMC ready event to COME (COME A85)
setup_gpio FM_BMC_READY_R_L                 GPIOP0 out 0

# To enable the path to access COMe FPGA and SCMCPLD
setup_gpio BMC_I2C1_EN                      GPIOG0 out 0
setup_gpio BMC_I2C2_EN                      GPIOF1 out 0

# JTAG MUX
setup_gpio MUX_JTAG_SEL_0                   GPIOF0 out 0
setup_gpio MUX_JTAG_SEL_1                   GPIOF5 out 0

# Mux select signal for BMC to program IOB flash (SPI)
setup_gpio IOB_FLASH_SEL                    GPIOG4 out 0

# SPI Mux select to COME (B89) and PROT module
setup_gpio SPI_MUX_SEL                      GPIOQ6 out 0
