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

# GPIO1 (SoC1)
setup_gpio VRHOT_L                 GPIOB0 in
setup_gpio CPU_MSMI_L              GPIOB3 in
setup_gpio CPU_RESET_L             GPIOB5 in
setup_gpio CP_PWR_ON               GPIOB6 in
setup_gpio SW_CPLD_JTAG_SEL        GPIOB7 out 0
setup_gpio DPM_CP_PGOOD            GPIOC0 in
setup_gpio CPLD_2_BMC_INTR         GPIOC1 in
setup_gpio CPU_CATERR_L            GPIOC2 in
setup_gpio BMC_MODE                GPIOC4 in
setup_gpio EPHY_INT_L              GPIOC5 in
setup_gpio AST_IDPROM_WP           GPIOC7 out 1
setup_gpio SICSID_ALERT_L          GPIOD1 in
setup_gpio OVERTEMP_L              GPIOD4 in
setup_gpio USB_DONGLE_PRSNT        GPIOD7 in
setup_gpio SYS_RESET               GPIOF0 in
setup_gpio BMC_WDTRST1             GPIOF1 in
setup_gpio CPU_OT_L                GPIOF4 in
setup_gpio DPM_APU_SYS_PWRGOOD     GPIOL0 in
setup_gpio BUF_BFLSH_WP_L          GPIOL1 out 0
setup_gpio BMC_ALIVE               GPIOL2 out 1
setup_gpio CPU_OVER_TEMP           GPIOL3 in
setup_gpio MSW_INTR_L              GPIOL4 in
setup_gpio SWC_CP_PWR_OK           GPIOL5 in
setup_gpio CPU_JTAG_SEL            GPIOL6 out 0
setup_gpio ABOOT_GRAB              GPIOL7 out 0
setup_gpio SW_SPI_WP_L             GPIOO3 out 0
setup_gpio SW_SPI_HOLD_L           GPIOO4 out 1
setup_gpio BMC_LITE_L              GPIOS7 out 0
setup_gpio BMC_SPI_2_CPLD          GPIOV0 out 0
setup_gpio SPI_ROM_REQ             GPIOAA3 in
