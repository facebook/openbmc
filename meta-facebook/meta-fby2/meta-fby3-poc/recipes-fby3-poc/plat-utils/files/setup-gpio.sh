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

# The commented-out sections are generally already defined elsewhere,
# and defining them twice generates errors.

# The exception to this is the definition of the GPIO H0, H1, and H2
# pins, which seem to adversely affect the rebooting of the system.
# When defined, the system doesn't reboot cleanly.  We're still
# investigating this.

. /usr/local/fbpackages/utils/ast-functions

echo "Set up GPIO pins....."

# GPIOA0-BMC_READY_R
devmem_clear_bit $(scu_addr 80) 0
gpio_export BMC_READY_R GPIOA0
gpio_set BMC_READY_R 1

# GPIOA1-SMB_HOTSWAP_BMC_ALERT_N_R
devmem_clear_bit $(scu_addr 80) 1
gpio_export SMB_HOTSWAP_BMC_ALERT_N_R GPIOA1

# GPIOA2-FM_HSC_BMC_FAULT_N_R 
devmem_clear_bit $(scu_addr 80) 15
gpio_export FM_HSC_BMC_FAULT_N_R GPIOA2

# GPIOA3-FM_NIC_WAKE_BMC_N
devmem_clear_bit $(scu_addr 80) 3
gpio_export FM_NIC_WAKE_BMC_N GPIOA3

# GPIOB0-SMB_BMC_SLOT1_ALT_N
# GPIOB1-SMB_BMC_SLOT2_ALT_N
# GPIOB2-SMB_BMC_SLOT3_ALT_N
# GPIOB3-SMB_BMC_SLOT4_ALT_N
gpio_export SMB_BMC_SLOT1_ALT_N GPIOB0
gpio_export SMB_BMC_SLOT2_ALT_N GPIOB1
gpio_export SMB_BMC_SLOT3_ALT_N GPIOB2
gpio_export SMB_BMC_SLOT4_ALT_N GPIOB3

# GPIOB4-PRSNT_MB_BMC_SLOT1_BB_N
# GPIOB5-PRSNT_MB_BMC_SLOT2_BB_N
# GPIOB6-PRSNT_MB_BMC_SLOT3_BB_N
# GPIOB7-PRSNT_MB_BMC_SLOT4_BB_N
devmem_clear_bit $(scu_addr 80) 13
devmem_clear_bit $(scu_addr 80) 14
val=$(/sbin/devmem 0x1e780004)
val=$((val & ~(0xf << 12)))
/sbin/devmem 0x1e780004 32 $val
gpio_export PRSNT_MB_BMC_SLOT1_BB_N GPIOB4
gpio_export PRSNT_MB_BMC_SLOT2_BB_N GPIOB5
gpio_export PRSNT_MB_BMC_SLOT3_BB_N GPIOB6
gpio_export PRSNT_MB_BMC_SLOT4_BB_N GPIOB7

# GPIOD0-EMMC_SD2CLK_R1  (function pin)
# GPIOD1-EMMC_SD2CMD_R1  (function pin)
# GPIOD2-EMMC_SD2DAT0_R1 (function pin)
# GPIOD3-EMMC_SD2DAT0_R1 (function pin)
# GPIOD4-EMMC_SD2DAT0_R1 (function pin)
# GPIOD5-EMMC_SD2DAT0_R1 (function pin)
# GPIOD6-SD2DET_N (function pin)
# GPIOD7-SD2WPT_N (function pin)
devmem_set_bit $(scu_addr 90) 1

# GPIOE0-ADAPTER_CARD_TYPE_BMC_1 
# GPIOE1-ADAPTER_CARD_TYPE_BMC_2
# GPIOE2-ADAPTER_CARD_TYPE_BMC_3
devmem_clear_bit $(scu_addr 80) 16
devmem_clear_bit $(scu_addr 80) 17
devmem_clear_bit $(scu_addr 80) 18
devmem_clear_bit $(scu_addr 8C) 12
devmem_clear_bit $(scu_addr 8C) 13
gpio_export ADAPTER_CARD_TYPE_BMC_1 GPIOE0
gpio_export ADAPTER_CARD_TYPE_BMC_2 GPIOE1
gpio_export ADAPTER_CARD_TYPE_BMC_3 GPIOE2

# GPIOE3-ADAPTER_BUTTON_BMC_CO_N_R
devmem_clear_bit $(scu_addr 80) 19
gpio_export ADAPTER_BUTTON_BMC_CO_N_R GPIOE3

# GPIOE4-RST_BMC_USB_HUB_N_R
devmem_clear_bit $(scu_addr 80) 20
devmem_clear_bit $(scu_addr 8C) 14
gpio_export RST_BMC_USB_HUB_N_R GPIOE4
gpio_set RST_BMC_USB_HUB_N_R 0

# GPIOE5-EMMC_RST_N_R
devmem_clear_bit $(scu_addr 80) 21
gpio_export EMMC_RST_N_R GPIOE5
gpio_set EMMC_RST_N_R 0

# GPIOE6-UART_BMC_SLOT3_RX_R
# GPIOE7-UART_BMC_SLOT3_TX
devmem_set_bit $(scu_addr 80) 22
devmem_set_bit $(scu_addr 80) 23

# GPIOF0-HSC_BMC_SLOT1_EN_R
# GPIOF1-HSC_BMC_SLOT2_EN_R
# GPIOF2-HSC_BMC_SLOT3_EN_R
# GPIOF3-HSC_BMC_SLOT4_EN_R
devmem_clear_bit $(scu_addr 80) 24
devmem_clear_bit $(scu_addr 80) 25
devmem_clear_bit $(scu_addr 80) 26
devmem_clear_bit $(scu_addr 80) 27
devmem_clear_bit $(scu_addr 90) 30
gpio_export HSC_BMC_SLOT1_EN_R GPIOF0 
gpio_export HSC_BMC_SLOT2_EN_R GPIOF1 
gpio_export HSC_BMC_SLOT3_EN_R GPIOF2 
gpio_export HSC_BMC_SLOT4_EN_R GPIOF3 
gpio_set HSC_BMC_SLOT1_EN_R 0
gpio_set HSC_BMC_SLOT2_EN_R 0
gpio_set HSC_BMC_SLOT3_EN_R 0
gpio_set HSC_BMC_SLOT4_EN_R 0

# GPIOF4-SMB_RST_PRIMARY_BMC_N_R
devmem_clear_bit $(scu_addr 80) 28
gpio_export SMB_RST_PRIMARY_BMC_N_R GPIOF4 
gpio_set SMB_RST_PRIMARY_BMC_N_R 1

# GPIOF6-UART_BMC_SLOT4_RX_R
# GPIOF7-UART_BMC_SLOT4_CO_TX
devmem_set_bit $(scu_addr 80) 30
devmem_set_bit $(scu_addr 80) 31

# GPIOG0-DISABLE_BMC_FAN_2_N_R
devmem_clear_bit $(scu_addr 84) 0
gpio_export DISABLE_BMC_FAN_2_N_R GPIOG0
gpio_set DISABLE_BMC_FAN_2_N_R 1

# GPIOG1-DISABLE_BMC_FAN_3_N_R
devmem_clear_bit $(scu_addr 84) 1
gpio_export DISABLE_BMC_FAN_3_N_R GPIOG1
gpio_set DISABLE_BMC_FAN_3_N_R 1

# GPIOG2-NIC_ADAPTER_CARD_PRSNT_BMC_1_N_R
devmem_clear_bit $(scu_addr 84) 2
gpio_export NIC_ADAPTER_CARD_PRSNT_BMC_1_N_R GPIOG2

# GPIOG3-SMB_RST_SECONDARY_BMC_N_R
devmem_clear_bit $(scu_addr 84) 3
gpio_export SMB_RST_SECONDARY_BMC_N_R GPIOG3
gpio_set SMB_RST_SECONDARY_BMC_N_R 1

# GPIOH6-UART_NIC_TX_R (function pin)
# GPIOH7-UART_NIC_RX (function pin)
devmem_clear_bit $(scu_addr 90) 6
devmem_set_bit $(scu_addr 90) 7

# GPIOJ4-RST_PCIE_RESET_SLOT1_N
# GPIOJ5-RST_PCIE_RESET_SLOT2_N
# GPIOJ6-RST_PCIE_RESET_SLOT3_N
# GPIOJ7-RST_PCIE_RESET_SLOT4_N
devmem_clear_bit $(scu_addr 84) 12
devmem_clear_bit $(scu_addr 84) 13
devmem_clear_bit $(scu_addr 84) 14
devmem_clear_bit $(scu_addr 84) 15
devmem_clear_bit $(scu_addr 94) 8
devmem_clear_bit $(scu_addr 94) 9
gpio_export RST_PCIE_RESET_SLOT1_N GPIOJ4
gpio_export RST_PCIE_RESET_SLOT2_N GPIOJ5
gpio_export RST_PCIE_RESET_SLOT3_N GPIOJ6
gpio_export RST_PCIE_RESET_SLOT4_N GPIOJ7

# GPIOL0-AC_ON_OFF_BTN_SLOT1_N
# GPIOL1-AC_ON_OFF_BTN_BMC_SLOT2_N_R
# GPIOL2-AC_ON_OFF_BTN_SLOT3_N
# GPIOL3-AC_ON_OFF_BTN_BMC_SLOT4_N_R
devmem_clear_bit $(scu_addr 84) 16
devmem_clear_bit $(scu_addr 84) 17
devmem_clear_bit $(scu_addr 84) 18
devmem_clear_bit $(scu_addr 84) 19
gpio_export AC_ON_OFF_BTN_SLOT1_N GPIOL0
gpio_export AC_ON_OFF_BTN_BMC_SLOT2_N_R GPIOL1
gpio_export AC_ON_OFF_BTN_SLOT3_N GPIOL2
gpio_export AC_ON_OFF_BTN_BMC_SLOT4_N_R GPIOL3

# GPIOL4-FAST_PROCHOT_BMC_N_R
devmem_clear_bit $(scu_addr 84) 20
gpio_export FAST_PROCHOT_BMC_N_R GPIOL4

# GPIOL5-USB_BMC_EN_R
devmem_clear_bit $(scu_addr 84) 21
gpio_export USB_BMC_EN_R GPIOL5
gpio_set USB_BMC_EN_R 1

# GPIOL6-UART_BMC_SLOT1_RX_R
# GPIOL7-UART_BMC_SLOT1_TX
devmem_set_bit $(scu_addr 84) 22
devmem_set_bit $(scu_addr 84) 23

# GPIOM0-HSC_FAULT_SLOT1_N
# GPIOM1-HSC_FAULT_BMC_SLOT2_N_R
# GPIOM2-HSC_FAULT_SLOT3_N
# GPIOM3-HSC_FAULT_BMC_SLOT4_N_R
devmem_clear_bit $(scu_addr 84) 24
devmem_clear_bit $(scu_addr 84) 25
devmem_clear_bit $(scu_addr 84) 26
devmem_clear_bit $(scu_addr 84) 27
gpio_export HSC_FAULT_SLOT1_N GPIOM0
gpio_export HSC_FAULT_BMC_SLOT2_N_R GPIOM1
gpio_export HSC_FAULT_SLOT3_N GPIOM2
gpio_export HSC_FAULT_BMC_SLOT4_N_R GPIOM3

# GPIOM4-FM_PWRBRK_PRIMARY_R
# GPIOM5-FM_PWRBRK_SECONDARY_R
devmem_clear_bit $(scu_addr 84) 28
devmem_clear_bit $(scu_addr 84) 29
gpio_export FM_PWRBRK_PRIMARY_R GPIOM4
gpio_set FM_PWRBRK_PRIMARY_R 0

gpio_export FM_PWRBRK_SECONDARY_R GPIOM5
gpio_set FM_PWRBRK_SECONDARY_R 0

# GPIOM6-UART_BMC_SLOT2_RX_R
# GPIOM7-UART_BMC_SLOT2_CO_TX
devmem_set_bit $(scu_addr 84) 30
devmem_set_bit $(scu_addr 84) 31

# GPION0-FAN_BMC_PWM_0
# GPION1-FAN_BMC_PWM_1
# GPION3-FAN_BMC_PWM_2
# GPION4-FAN_BMC_PWM_3
devmem_set_bit $(scu_addr 88) 0
devmem_set_bit $(scu_addr 88) 1
devmem_set_bit $(scu_addr 88) 2
devmem_set_bit $(scu_addr 88) 3

# GPION4-DUAL_FAN0_DETECT_BMC_N_R
# GPION5-DUAL_FAN1_DETECT_BMC_N_R
devmem_clear_bit $(scu_addr 88) 4
devmem_clear_bit $(scu_addr 88) 5
gpio_export DUAL_FAN0_DETECT_BMC_N_R GPION4
gpio_export DUAL_FAN1_DETECT_BMC_N_R GPION5

# GPION6-DISABLE_BMC_FAN_0_N_R
devmem_clear_bit $(scu_addr 88) 6
gpio_export DISABLE_BMC_FAN_0_N_R GPION6
gpio_set DISABLE_BMC_FAN_0_N_R 1

# GPION7-DISABLE_BMC_FAN_1_N_R
devmem_clear_bit $(scu_addr 88) 7
gpio_export DISABLE_BMC_FAN_1_N_R GPION7
gpio_set DISABLE_BMC_FAN_1_N_R 1

# Make sure the setting is correct.
# GPIOO0-FAN_BMC_TACH_0
# GPIOO1-FAN_BMC_TACH_1
# GPIOO2-FAN_BMC_TACH_2
# GPIOO3-FAN_BMC_TACH_3
# GPIOO4-FAN_BMC_TACH_4
# GPIOO5-FAN_BMC_TACH_5
# GPIOO6-FAN_BMC_TACH_6
# GPIOO7-FAN_BMC_TACH_7
devmem_clear_bit $(scu_addr 88) 8
devmem_clear_bit $(scu_addr 88) 9
devmem_clear_bit $(scu_addr 88) 10
devmem_clear_bit $(scu_addr 88) 11
devmem_clear_bit $(scu_addr 88) 12
devmem_clear_bit $(scu_addr 88) 13
devmem_clear_bit $(scu_addr 88) 14
devmem_clear_bit $(scu_addr 88) 15
val=$(/sbin/devmem 0x1e78007c)
val=$((val & ~(0xf << 20)))
/sbin/devmem 0x1e78007c 32 $val

# GPIOQ6-USB_OC_N
devmem_clear_bit $(scu_addr 2C) 1
gpio_export USB_OC_N GPIOQ6

# GPIOQ7-FM_BMC_TPM_PRSNT_N
devmem_clear_bit $(scu_addr 2C) 29
gpio_export FM_BMC_TPM_PRSNT_N GPIOQ7

# GPIOS0-PWROK_STBY_BMC_SLOT1
# GPIOS1-PWROK_STBY_BMC_SLOT2
# GPIOS2-PWROK_STBY_BMC_SLOT3
# GPIOS3-PWROK_STBY_BMC_SLOT4
devmem_clear_bit $(scu_addr 8C) 0
devmem_clear_bit $(scu_addr 8C) 1
devmem_clear_bit $(scu_addr 8C) 2
devmem_clear_bit $(scu_addr 8C) 3
gpio_export PWROK_STBY_BMC_SLOT1 GPIOS0
gpio_export PWROK_STBY_BMC_SLOT2 GPIOS1
gpio_export PWROK_STBY_BMC_SLOT3 GPIOS2
gpio_export PWROK_STBY_BMC_SLOT4 GPIOS3

# GPIOZ0-FM_BMC_SLOT1_ISOLATED_EN_R
# GPIOZ1-FM_BMC_SLOT2_ISOLATED_EN_R
# GPIOZ2-FM_BMC_SLOT3_ISOLATED_EN_R
# GPIOZ3-FM_BMC_SLOT4_ISOLATED_EN_R
devmem_clear_bit $(scu_addr A4) 16
devmem_clear_bit $(scu_addr A4) 17
devmem_clear_bit $(scu_addr A4) 18
devmem_clear_bit $(scu_addr A4) 19
gpio_export FM_BMC_SLOT1_ISOLATED_EN_R GPIOZ0
gpio_export FM_BMC_SLOT2_ISOLATED_EN_R GPIOZ1
gpio_export FM_BMC_SLOT3_ISOLATED_EN_R GPIOZ2
gpio_export FM_BMC_SLOT4_ISOLATED_EN_R GPIOZ3
gpio_set FM_BMC_SLOT1_ISOLATED_EN_R 0
gpio_set FM_BMC_SLOT2_ISOLATED_EN_R 0
gpio_set FM_BMC_SLOT3_ISOLATED_EN_R 0
gpio_set FM_BMC_SLOT4_ISOLATED_EN_R 0

# GPIOAB0-PWRGD_NIC_BMC
devmem_clear_bit $(scu_addr A8) 0
gpio_export PWRGD_NIC_BMC GPIOAB0

# GPIOAB1-OCP_DEBUG_BMC_PRSNT_N
devmem_clear_bit $(scu_addr A8) 1
gpio_export OCP_DEBUG_BMC_PRSNT_N GPIOAB1

# GPIOAC4-SMB_FAN_INA_BMC_ALERT_N_R
# GPIOAC5-SMB_TEMP_INLET_ALERT_BMC_N_R
# GPIOAC6-SMB_TEMP_OUTLET_ALERT_BMC_N_R
# GPIOAC7-FM_BMC_BIC_DETECT_N
devmem_clear_bit $(scu_addr AC) 4
devmem_clear_bit $(scu_addr AC) 5
devmem_clear_bit $(scu_addr AC) 6
devmem_clear_bit $(scu_addr AC) 7
gpio_export SMB_FAN_INA_BMC_ALERT_N_R GPIOAC4
gpio_export SMB_TEMP_INLET_ALERT_BMC_N_R GPIOAC5
gpio_export SMB_TEMP_OUTLET_ALERT_BMC_N_R GPIOAC6
gpio_export FM_BMC_BIC_DETECT_N GPIOAC7
