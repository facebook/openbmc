#!/bin/bash
#
# Copyright 2022-present Facebook. All Rights Reserved.
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

# ==GPIOA==


# ==GPIOB==
# FM_ME_BT_DONE
#gpio_export FM_ME_BT_DONE GPIOB0

# FM_BMC_MUX_CS_SPI_SEL_0
gpio_export FM_BMC_MUX_CS_SPI_SEL_0 GPIOB4
gpio_set FM_BMC_MUX_CS_SPI_SEL_0 0

# FM_ID_LED_N
gpio_export FM_ID_LED_N GPIOB5
gpio_set FM_ID_LED_N 0

# FM_ESPI_PLD_EN
gpio_export FM_ESPI_PLD_EN GPIOB6

# ==GPIOC==
# RST_PLTRST_R_N
gpio_export RST_PLTRST_R_N GPIOC0

#RST_SGPIO_RESET_BMC_N
gpio_export RST_SGPIO_RESET_BMC_N GPIOC3
gpio_set RST_SGPIO_RESET_BMC_N 1

#H_CPU_CATERR_LVC2_R2_N
gpio_export H_CPU_CATERR_LVC2_R2_N GPIOC5

#FM_JTAG_BMC_MUX_SEL
gpio_export FM_JTAG_BMC_MUX_SEL GPIOC6
gpio_set FM_JTAG_BMC_MUX_SEL 1;

#IRQ_SGPIO_BMC_N
gpio_export IRQ_SGPIO_BMC_N GPIOC7

# ==GPIOD==
gpio_export FM_BIC_DEBUG_SW_N GPIOD2
gpio_set FM_BIC_DEBUG_SW_N 0

# ==GPIOE==
#FM_FLASH_LATCH_N_R1
gpio_export FM_FLASH_LATCH_N_R1 GPIOE0

#FLASH_WP_STATUS_R1
gpio_export FLASH_WP_STATUS_R1 GPIOE1

# ==GPIOF==

# ==GPIOG==

# ==GPIOH==

# ==GPIOI==
# JTAG_1_BMC_TRST_R
gpio_export JTAG_1_BMC_TRST_R GPIOI0

# PD_BIOS_MUX_EN_N
gpio_export PD_BIOS_MUX_EN_N GPIOI5
gpio_set PD_BIOS_MUX_EN_N 0

# ==GPIOJ==


# ==GPIOK==


# ==GPIOL==
# FM_DBP_CPU_PREQ_GF_N
gpio_export FM_DBP_CPU_PREQ_GF_N GPIOL4
gpio_set FM_DBP_CPU_PREQ_GF_N 1


# ==GPIOM==
# FM_VIRTUAL_RESEAT_BMC
gpio_export FM_VIRTUAL_RESEAT_BMC GPIOM1

# PWRGD_PSU_AC_PWROK_BMC
gpio_export PWRGD_PSU_AC_PWROK_BMC GPIOM2

# FM_DBP_BMC_PRDY_N
gpio_export FM_DBP_BMC_PRDY_N GPIOM5


# ==GPION==
# LED_POSTCODE_0
gpio_export LED_POSTCODE_0 GPION0

# LED_POSTCODE_1
gpio_export LED_POSTCODE_1 GPION1

# LED_POSTCODE_2
gpio_export LED_POSTCODE_2 GPION2

# LED_POSTCODE_3
gpio_export LED_POSTCODE_3 GPION3

# LED_POSTCODE_4
gpio_export LED_POSTCODE_4 GPION4

# LED_POSTCODE_5
gpio_export LED_POSTCODE_5 GPION5

# LED_POSTCODE_6
gpio_export LED_POSTCODE_6 GPION6

# LED_POSTCODE_7
gpio_export LED_POSTCODE_7 GPION7


# ==GPIOO==
# FM_PCH_TPM_PRSNT
gpio_export FM_PCH_TPM_PRSNT GPIOO5

# ==GPIOP==
# SYS_BMC_PWRBTN_IN
gpio_export SYS_BMC_PWRBTN_IN GPIOP0

# SYS_BMC_PWRBTN_OUT
gpio_export SYS_BMC_PWRBTN_OUT GPIOP1
gpio_set SYS_BMC_PWRBTN_OUT 1

# SYS_BMC_RSTBTN_IN
gpio_export ID_RST_BTN_BMC_IN GPIOP2

# BMC_PWR_LED
gpio_export BMC_PWR_LED GPIOP4
gpio_set BMC_PWR_LED 0

# ==GPIOQ==
# IRQ_PCH_TPM_SPI_IN
gpio_export IRQ_PCH_TPM_SPI_IN GPIOQ0

# USB_OC0_REAR_R_IN
gpio_export USB_OC0_REAR_R_IN GPIOQ1

# FM_HDD_LED_OUT
gpio_export FM_HDD_LED_N_OUT GPIOQ4
gpio_set FM_HDD_LED_N_OUT 0

# FM_P12V_HSC_ENABLE_R1
gpio_export FM_P12V_HSC_ENABLE_R1 GPIOQ5
gpio_set FM_P12V_HSC_ENABLE_R1 1

# FM_PCH_BEEP_LED
gpio_export FM_PCH_BEEP_LED_OUT GPIOQ6
gpio_set FM_PCH_BEEP_LED_OUT 0

# FM_SOL_UART_CH_SEL
gpio_export FM_SOL_UART_CH_SEL GPIOQ7

# ==GPIOR==


# ==GPIOS==
# FM_THERMTRIP_N
gpio_export FM_THERMTRIP_N GPIOS2

# FM_BMC_TPM_PRSNT
gpio_export FM_BMC_TPM_PRSNT GPIOS3

# FM_BMC_DEBUG_SW_N
gpio_export FM_BMC_DEBUG_SW_N GPIOS4


# ==GPIU==
# BSM_PRSNT_R_N
gpio_export BSM_PRSNT_R_N GPIOU4

# ==GPIOV==
# RST_RSMRST_R_N
gpio_export RST_RSMRST_R_N GPIOV1

gpio_export BMC_ID_BEEP_SEL_R1 GPIOV2
#gpio_set BMC_ID_BEEP_SEL_R1 1

# BATTERY_DETECT
gpio_export BATTERY_DETECT GPIOV4
gpio_set BATTERY_DETECT 0

# IRQ_BMC_TPM_SPI_IN
gpio_export IRQ_BMC_TPM_SPI_IN GPIOV7

# ==GPIX==
# IRQ_SMI_ACTIVE_BMC_N
gpio_export IRQ_SMI_ACTIVE_BMC_N GPIOX2

# ==GPIOY==
gpio_export RST_WDTRST_PLD_R1_N GPIOY1

#FM BMC_EMMC_RST_R_N
gpio_export BMC_EMMC_RST_R_N GPIOY3

# ==GPIOZ==
# BMC_READY_N
#gpio_export BMC_READY_N GPIOZ0
#gpio_set BMC_READY_N 0

#FM_DEBUG_PORT_PRSNT_R1_N
gpio_export FM_DEBUG_PORT_PRSNT_N_IN GPIOZ6


# ==GPIO18D==
# EMMC Func GPIO18D0 - GPIO18D7

devmem_set_bit 0x1e7800e0 8
devmem_clear_bit 0x1e7000e4 8
