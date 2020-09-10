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
# Default-Start:     60 S
# Default-Stop:
# Short-Description:  Set up GPIO pins as appropriate
### END INIT INFO

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# Export GPIO
gpio_export_by_name "${ASPEED_GPIO}" GPIOB1 NP_POWER_STABLE_3V3_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOF3 SMB_CPLD_HITLESS_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOF6 BMC_EMMC_CD_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOF7 BMC_EMMC_WP_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOG2 CPLD_INT_BMC_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOG4 CPLD_RESERVED_2_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOG6 BMC_DOM_FPGA2_RST_L_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOG7 FCM_CARD_PRESENT
gpio_export_by_name "${ASPEED_GPIO}" GPIOH0 GPIO_H0
gpio_export_by_name "${ASPEED_GPIO}" GPIOH1 GPIO_H1
gpio_export_by_name "${ASPEED_GPIO}" GPIOH2 GPIO_H2
gpio_export_by_name "${ASPEED_GPIO}" GPIOH3 GPIO_H3
gpio_export_by_name "${ASPEED_GPIO}" GPIOH4 GPIO_H4
gpio_export_by_name "${ASPEED_GPIO}" GPIOH5 GPIO_H5
gpio_export_by_name "${ASPEED_GPIO}" GPIOM1 BMC_UART_SEL_5
gpio_export_by_name "${ASPEED_GPIO}" GPIOM3 SCM_CPLD_HITLESS_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOO0 PWR_CPLD_JTAG_EN_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOO1 BMC_JTAG_MUX_IN_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOO2 FCM_CPLD_JTAG_EN_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOO3 SYS_CPLD_JTAG_EN_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOO4 DOM_FPGA1_JTAG_EN_N_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOO5 GPIO_H6
gpio_export_by_name "${ASPEED_GPIO}" GPIOO6 SCM_CPLD_JTAG_EN_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOO7 GPIO_H7
gpio_export_by_name "${ASPEED_GPIO}" GPIOP0 BMC_UART_SEL_2
gpio_export_by_name "${ASPEED_GPIO}" GPIOP6 BMC_CPLD_SPARE3_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOQ0 FCM_CPLD_HITLESS_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOQ2 PWR_CPLD_HITLESS_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOQ3 BMC_IN_P1220_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOQ4 BMC_FCM_SEL_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOQ6 BMC_CPLD_SPARE4_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOQ7 SYS_CPLD_RST_L_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOR0 BMC_CPLD_SPARE2_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOR1 DOM_FPGA2_JTAG_EN_N_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOR2 BMC_CPLD_SPARE1_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOR3 DEBUG_PRESENT_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOR4 BMC_SCM_CPLD_EN_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOR5 BMC_CPLD_TPM_RST_L_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOR6 BMC_DOM_FPGA1_RST_L_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOV1 CPLD_RESERVED_3_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOV2 CPLD_RESERVED_4_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOV3 BMC_P1220_SYS_OK
gpio_export_by_name "${ASPEED_GPIO}" GPIOV4 BMC_POWER_OK
gpio_export_by_name "${ASPEED_GPIO}" GPIOW0 SCM_LPC_AD_0
gpio_export_by_name "${ASPEED_GPIO}" GPIOW1 SCM_LPC_AD_1
gpio_export_by_name "${ASPEED_GPIO}" GPIOW2 SCM_LPC_AD_2
gpio_export_by_name "${ASPEED_GPIO}" GPIOW3 SCM_LPC_AD_3
gpio_export_by_name "${ASPEED_GPIO}" GPIOW4 SCM_LPC_CLK
gpio_export_by_name "${ASPEED_GPIO}" GPIOW5 SCM_LPC_FRAME_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOW6 SCM_LPC_SERIRQ_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOW7 BMC_LPCRST_N
gpio_export_by_name "${ASPEED_GPIO}" GPIOX6 BMC_FPGA_JTAG_EN
gpio_export_by_name "${ASPEED_GPIO}" GPIOY2 CPU_CATERR_MSMI_R
gpio_export_by_name "${ASPEED_GPIO}" GPIOY3 SYS_CPLD_INT_L_R

# Set GPIO direction
gpio_set_direction NP_POWER_STABLE_3V3_R    "in"
gpio_set_direction SMB_CPLD_HITLESS_R       "out"
gpio_set_direction BMC_EMMC_CD_N            "in"
gpio_set_direction BMC_EMMC_WP_N            "in"
gpio_set_direction CPLD_INT_BMC_R           "in"
gpio_set_direction CPLD_RESERVED_2_R        "in"
gpio_set_direction BMC_DOM_FPGA2_RST_L_R    "out"
gpio_set_direction FCM_CARD_PRESENT         "in"
gpio_set_direction GPIO_H0                  "in"
gpio_set_direction GPIO_H1                  "in"
gpio_set_direction GPIO_H2                  "in"
gpio_set_direction GPIO_H3                  "in"
gpio_set_direction GPIO_H4                  "in"
gpio_set_direction GPIO_H5                  "in"
gpio_set_direction BMC_UART_SEL_5           "in"
gpio_set_direction SCM_CPLD_HITLESS_R       "out"
gpio_set_direction PWR_CPLD_JTAG_EN_N       "out"
gpio_set_direction BMC_JTAG_MUX_IN_R        "out"
gpio_set_direction FCM_CPLD_JTAG_EN_N       "out"
gpio_set_direction SYS_CPLD_JTAG_EN_N       "out"
gpio_set_direction DOM_FPGA1_JTAG_EN_N_R    "out"
gpio_set_direction GPIO_H6                  "in"
gpio_set_direction SCM_CPLD_JTAG_EN_N       "out"
gpio_set_direction GPIO_H7                  "in"
gpio_set_direction BMC_UART_SEL_2           "in"
gpio_set_direction BMC_CPLD_SPARE3_R        "in"
gpio_set_direction FCM_CPLD_HITLESS_R       "out"
gpio_set_direction PWR_CPLD_HITLESS_R       "out"
gpio_set_direction BMC_IN_P1220_R           "in"
gpio_set_direction BMC_FCM_SEL_R            "out"
gpio_set_direction BMC_CPLD_SPARE4_R        "in"
gpio_set_direction SYS_CPLD_RST_L_R         "out"
gpio_set_direction BMC_CPLD_SPARE2_R        "in"
gpio_set_direction DOM_FPGA2_JTAG_EN_N_R    "out"
gpio_set_direction BMC_CPLD_SPARE1_R        "out"
gpio_set_direction DEBUG_PRESENT_N          "in"
gpio_set_direction BMC_SCM_CPLD_EN_R        "out"
gpio_set_direction BMC_CPLD_TPM_RST_L_R     "out"
gpio_set_direction BMC_DOM_FPGA1_RST_L_R    "out"
gpio_set_direction CPLD_RESERVED_3_R        "in"
gpio_set_direction CPLD_RESERVED_4_R        "in"
gpio_set_direction BMC_P1220_SYS_OK         "in"
gpio_set_direction BMC_POWER_OK             "in"
gpio_set_direction SCM_LPC_AD_0             "in"
gpio_set_direction SCM_LPC_AD_1             "in"
gpio_set_direction SCM_LPC_AD_2             "in"
gpio_set_direction SCM_LPC_AD_3             "in"
gpio_set_direction SCM_LPC_CLK              "in"
gpio_set_direction SCM_LPC_FRAME_N          "in"
gpio_set_direction SCM_LPC_SERIRQ_N         "in"
gpio_set_direction BMC_LPCRST_N             "in"
gpio_set_direction CPU_CATERR_MSMI_R        "in"
gpio_set_direction SYS_CPLD_INT_L_R         "in"
gpio_set_direction BMC_FPGA_JTAG_EN         "out"

# Set GPIO default value
# Default set to 1. RST signal is active low, set it to high for normal operation
gpio_set_value BMC_DOM_FPGA2_RST_L_R    1
# Default set to 1. Enable signal active low, set it to high for normal operation(No JTAG burn in)
gpio_set_value PWR_CPLD_JTAG_EN_N       1
# Default set to 1. Enable signal active low, set it to high for normal operation(No JTAG burn in)
gpio_set_value BMC_FPGA_JTAG_EN         1
gpio_set_value BMC_JTAG_MUX_IN_R        0
# Default set to 0. Use header for CPLD FW burn in as default setting
gpio_set_value FCM_CPLD_JTAG_EN_N       1
# Default set to 1. Enable signal active low, set it to high for normal operation(No JTAG burn in)
gpio_set_value SYS_CPLD_JTAG_EN_N       1
# Default set to 1. Enable signal active low, set it to high for normal operation(No JTAG burn in)
gpio_set_value DOM_FPGA1_JTAG_EN_N_R    1
# Default set to 1. Enable signal active low, set it to high for normal operation(No JTAG burn in)
gpio_set_value SCM_CPLD_JTAG_EN_N       1
# Default set to 1. Use header for SCM CPLD FW burn in as default setting
gpio_set_value BMC_FCM_SEL_R            0
# Default set to 1. RST signal is active low, set it to high for normal operation
gpio_set_value SYS_CPLD_RST_L_R         1
# Default set to 1. Enable signal active low, set it to high for normal operation(No JTAG burn in)
gpio_set_value DOM_FPGA2_JTAG_EN_N_R    1
# Default set to 1. Use header for SCM CPLD FW burn in as default setting
gpio_set_value BMC_SCM_CPLD_EN_R        1
# Default set to 1. RST signal is active low, set it to high for normal operation
gpio_set_value BMC_DOM_FPGA1_RST_L_R    1

