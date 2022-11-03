#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
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


# FM_BMC_PWRBTN_OUT_N
gpio_export_by_name  "${ASPEED_GPIO}" GPIOP0 FM_BMC_PWRBTN_OUT_N
gpio_set_value FM_BMC_PWRBTN_OUT_N 1

# GPIOP1
# set GPIOP1 output high to prevent GPIOP0 from being affected
# if GPIOP1 is set to input, GPIOP0 only 1.7V when set to output high
gpio_export_by_name  "${ASPEED_GPIO}" GPIOP1 GPIOP1
gpio_set_value GPIOP1 1

# FM_BMC_RSTBTN_OUT_N
gpio_export_by_name  "${ASPEED_GPIO}" GPIOP3 FM_BMC_RSTBTN_OUT_N
gpio_set_value FM_BMC_RSTBTN_OUT_N 1

# SYS_SKU_ID0_R
gpio_export_by_name  "${ASPEED_GPIO}" GPIOC4 SYS_SKU_ID0_R

# SYS_SKU_ID1_R
gpio_export_by_name  "${ASPEED_GPIO}" GPIOX2 SYS_SKU_ID1_R

# MB_P12V_HS_EN_N
gpio_export_by_name  "${ASPEED_GPIO}" GPIOC5 MB_P12V_HS_EN_N
gpio_set_value MB_P12V_HS_EN_N 0

# FM_BIC_DEBUG_SW_N_R
gpio_export_by_name  "${ASPEED_GPIO}" GPIOD2 FM_BIC_DEBUG_SW_N_R
gpio_set_value FM_BIC_DEBUG_SW_N_R 0

# PWRGD_CPUPWRGD_LVC1
gpio_export_by_name  "${ASPEED_GPIO}" GPIOR6 PWRGD_CPUPWRGD_LVC1

# BIC_EXTRST
gpio_export_by_name  "${ASPEED_GPIO}" GPIOS6 BIC_EXTRST
gpio_set_value BIC_EXTRST 1

# BIC_FWSPICK
gpio_export_by_name  "${ASPEED_GPIO}" GPIOD0 BIC_FWSPICK
gpio_set_value BIC_FWSPICK 0

# BIC_SRST
gpio_export_by_name  "${ASPEED_GPIO}" GPIOD1 BIC_SRST
gpio_set_value BIC_SRST 1

#SCB IO-Expender
gpio_export_by_offset 1-0021 0 BMC_UART_SEL0
gpio_set  BMC_UART_SEL0  0
gpio_export_by_offset 1-0021 1 BMC_UART_SEL1
gpio_set  BMC_UART_SEL1  0
gpio_export_by_offset 1-0021 2 BMC_UART_SEL2
gpio_set  BMC_UART_SEL2  0
gpio_export_by_offset 1-0021 3 BMC_UART_SEL3
gpio_set  BMC_UART_SEL3  0
gpio_export_by_offset 1-0021 4 BMC_UART_SEL4
gpio_export_by_offset 1-0021 5 BMC_UART_MUX_SEL_N
gpio_set  BMC_UART_MUX_SEL_N  0
gpio_export_by_offset 1-0021 6 FM_PWRGD_SCB_R_PWRGD_BUF
gpio_export_by_offset 1-0021 7 FP_PWR_LED_BMC
gpio_export_by_offset 1-0021 8 FP_STS_LED_BMC
gpio_export_by_offset 1-0021 9 BMC_FP_LED_MUX_SEL_N
gpio_export_by_offset 1-0021 10 PWR_BTN_QS_N_BMC
gpio_export_by_offset 1-0021 11 RST_BTN_QS_N_BMC
gpio_export_by_offset 1-0021 12 DC_SCM_FW_RDY_R_N



# FM_BMC_READY_PLD_R
gpio_export_by_name  "${ASPEED_GPIO}" GPIOV5 FM_BMC_READY_PLD_R
gpio_set_value FM_BMC_READY_PLD_R 0
