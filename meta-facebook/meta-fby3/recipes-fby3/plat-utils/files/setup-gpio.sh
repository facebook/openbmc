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

echo -n "Set up GPIO pins....."

# GPIOF0-BOARD_BMC_ID0_R
# GPIOF1-BOARD_BMC_ID1_R
# GPIOF2-BOARD_BMC_ID2_R
# GPIOF3-BOARD_BMC_ID3_R
devmem_clear_bit $(scu_addr 80) 24
devmem_clear_bit $(scu_addr 80) 25
devmem_clear_bit $(scu_addr 80) 26
devmem_clear_bit $(scu_addr 80) 27
devmem_clear_bit $(scu_addr 90) 30
gpio_export BOARD_BMC_ID0_R GPIOF0
gpio_export BOARD_BMC_ID1_R GPIOF1
gpio_export BOARD_BMC_ID2_R GPIOF2
gpio_export BOARD_BMC_ID3_R GPIOF3

BOARD_ID=$(get_bmc_board_id)

# GPIOA0-BMC_READY_R
devmem_clear_bit $(scu_addr 80) 0
gpio_export BMC_READY_R GPIOA0
gpio_set BMC_READY_R 0

# GPIOA3-FM_NIC_WAKE_BMC_N
devmem_clear_bit $(scu_addr 80) 3
gpio_export FM_NIC_WAKE_BMC_N GPIOA3

# GPIOE0-P3V3_NIC_FAULT_N
# GPIOE1-NIC_POWER_BMC_EN_R
# GPIOE2-P12V_NIC_FAULT_N
# GPIOE5-EMMC_RST_N_R
devmem_clear_bit $(scu_addr 80) 16
devmem_clear_bit $(scu_addr 80) 17
devmem_clear_bit $(scu_addr 80) 18
devmem_clear_bit $(scu_addr 80) 21
devmem_clear_bit $(scu_addr 8C) 12
devmem_clear_bit $(scu_addr 8C) 13
devmem_clear_bit $(scu_addr 8C) 14
gpio_export P3V3_NIC_FAULT_N GPIOE0
gpio_export NIC_POWER_BMC_EN_R GPIOE1
gpio_export P12V_NIC_FAULT_N GPIOE2
gpio_export EMMC_RST_N_R GPIOE5
gpio_set NIC_POWER_BMC_EN_R 1
gpio_set EMMC_RST_N_R 0

# GPIOF4-SMB_RST_PRIMARY_BMC_N_R
devmem_clear_bit $(scu_addr 80) 28
gpio_export SMB_RST_PRIMARY_BMC_N_R GPIOF4

# GPIOG3-SMB_RST_SECONDARY_BMC_N_R
devmem_clear_bit $(scu_addr 84) 3
gpio_export SMB_RST_SECONDARY_BMC_N_R GPIOG3

# GPIOM4-FM_PWRBRK_PRIMARY_R
# GPIOM5-FM_PWRBRK_SECONDARY_R
devmem_clear_bit $(scu_addr 84) 28
devmem_clear_bit $(scu_addr 84) 29
gpio_export FM_PWRBRK_PRIMARY_R GPIOM4
gpio_export FM_PWRBRK_SECONDARY_R GPIOM5
gpio_set FM_PWRBRK_PRIMARY_R 0
gpio_set FM_PWRBRK_SECONDARY_R 0

# GPIOQ7-FM_BMC_TPM_PRSNT_N
devmem_clear_bit $(scu_addr 2C) 29
gpio_export FM_BMC_TPM_PRSNT_N GPIOQ7

# GPIOAB0-PWRGD_NIC_BMC
devmem_clear_bit $(scu_addr A8) 0
gpio_export PWRGD_NIC_BMC GPIOAB0

# GPIOAB2-RST_BMC_WDRST1_R
devmem_clear_bit $(scu_addr 94) 0
devmem_clear_bit $(scu_addr 94) 1
devmem_set_bit $(scu_addr A8) 2
gpio_set RST_BMC_WDRST1_R GPIOAB2

# GPIOAB3-RST_BMC_WDRST2_R
devmem_set_bit $(scu_addr A8) 3
gpio_set RST_BMC_WDRST2_R GPIOAB3

# GPIOAC4-EMMC_PRESENT_N
devmem_clear_bit $(scu_addr AC) 4
gpio_export EMMC_PRESENT_N GPIOAC4

###NIC EXP BMC###
if [ $BOARD_ID -eq 9 ]; then
  # GPIOB0-SMB_MUX_ALT_N
  gpio_export SMB_MUX_ALT_N GPIOB0

  # GPIOS0-PWROK_STBY_BMC
  devmem_clear_bit $(scu_addr 8C) 0
  gpio_export PWROK_STBY_BMC GPIOS0

  # GPIOZ0-FM_BMC_ISOLATED_EN_R
  # GPIOZ1-FM_BMC_ISOLATED_UART_EN_R
  devmem_clear_bit $(scu_addr A4) 16
  devmem_clear_bit $(scu_addr A4) 17
  gpio_export FM_BMC_ISOLATED_EN_R GPIOZ0
  gpio_export FM_BMC_ISOLATED_UART_EN_R GPIOZ1
  gpio_set FM_BMC_ISOLATED_EN_R 1
  gpio_set FM_BMC_ISOLATED_UART_EN_R 0

###Baseboard BMC###
elif [ $BOARD_ID -eq 14 ] || [ $BOARD_ID -eq 7 ]; then
  # GPIOA1-SMB_HOTSWAP_BMC_ALERT_N_R
  devmem_clear_bit $(scu_addr 80) 1
  gpio_export SMB_HOTSWAP_BMC_ALERT_N_R GPIOA1

  # GPIOA2-FM_HSC_BMC_FAULT_N_R
  devmem_clear_bit $(scu_addr 80) 15
  gpio_export FM_HSC_BMC_FAULT_N_R GPIOA2

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
  gpio_export PRSNT_MB_BMC_SLOT1_BB_N GPIOB4
  gpio_export PRSNT_MB_BMC_SLOT2_BB_N GPIOB5
  gpio_export PRSNT_MB_BMC_SLOT3_BB_N GPIOB6
  gpio_export PRSNT_MB_BMC_SLOT4_BB_N GPIOB7

  # GPIOE3-ADAPTER_BUTTON_BMC_CO_N_R
  # GPIOE4-RST_BMC_USB_HUB_N_R
  devmem_clear_bit $(scu_addr 80) 19
  devmem_clear_bit $(scu_addr 80) 20
  gpio_export ADAPTER_BUTTON_BMC_CO_N_R GPIOE3
  gpio_export RST_BMC_USB_HUB_N_R GPIOE4
  gpio_set RST_BMC_USB_HUB_N_R 1

  # GPIOI0-FAN0_BMC_CPLD_EN_R
  # GPIOI1-FAN1_BMC_CPLD_EN_R
  # GPIOI2-FAN2_BMC_CPLD_EN_R
  # GPIOI3-FAN3_BMC_CPLD_EN_R
  devmem_clear_bit $(scu_addr 90) 6
  gpio_export FAN0_BMC_CPLD_EN_R GPIOI0
  gpio_export FAN1_BMC_CPLD_EN_R GPIOI1
  gpio_export FAN2_BMC_CPLD_EN_R GPIOI2
  gpio_export FAN3_BMC_CPLD_EN_R GPIOI3
  gpio_set FAN0_BMC_CPLD_EN_R 1
  gpio_set FAN1_BMC_CPLD_EN_R 1
  gpio_set FAN2_BMC_CPLD_EN_R 1
  gpio_set FAN3_BMC_CPLD_EN_R 1

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

  # GPION4-DUAL_FAN0_DETECT_BMC_N_R
  # GPION5-DUAL_FAN1_DETECT_BMC_N_R
  devmem_clear_bit $(scu_addr 88) 4
  devmem_clear_bit $(scu_addr 88) 5
  gpio_export DUAL_FAN0_DETECT_BMC_N_R GPION4
  gpio_export DUAL_FAN1_DETECT_BMC_N_R GPION5

  # GPIOP0-SLOT1_ID0_DETECT_BMC_N
  # GPIOP1-SLOT1_ID1_DETECT_BMC_N
  # GPIOP2-SLOT2_ID0_DETECT_BMC_N
  # GPIOP3-SLOT2_ID1_DETECT_BMC_N
  # GPIOP4-SLOT3_ID0_DETECT_BMC_N
  # GPIOP5-SLOT3_ID1_DETECT_BMC_N
  # GPIOP6-SLOT4_ID0_DETECT_BMC_N
  # GPIOP7-SLOT4_ID1_DETECT_BMC_N
  devmem_clear_bit $(scu_addr 88) 16
  devmem_clear_bit $(scu_addr 88) 17
  devmem_clear_bit $(scu_addr 88) 18
  devmem_clear_bit $(scu_addr 88) 19
  devmem_clear_bit $(scu_addr 88) 20
  devmem_clear_bit $(scu_addr 88) 21
  devmem_clear_bit $(scu_addr 88) 22
  devmem_clear_bit $(scu_addr 88) 23
  gpio_export SLOT1_ID0_DETECT_BMC_N GPIOP0
  gpio_export SLOT1_ID1_DETECT_BMC_N GPIOP1
  gpio_export SLOT2_ID0_DETECT_BMC_N GPIOP2
  gpio_export SLOT2_ID1_DETECT_BMC_N GPIOP3
  gpio_export SLOT3_ID0_DETECT_BMC_N GPIOP4
  gpio_export SLOT3_ID1_DETECT_BMC_N GPIOP5
  gpio_export SLOT4_ID0_DETECT_BMC_N GPIOP6
  gpio_export SLOT4_ID1_DETECT_BMC_N GPIOP7

  # GPIOQ6-USB_OC_N
  devmem_clear_bit $(scu_addr 2C) 1
  gpio_export USB_OC_N GPIOQ6

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

  # GPIOAB1-OCP_DEBUG_BMC_PRSNT_N
  devmem_clear_bit $(scu_addr A8) 1
  gpio_export OCP_DEBUG_BMC_PRSNT_N GPIOAB1

  # These pins are added in DVT
  # GPIOI4-FM_RESBTN_SLOT3_BMC_N
  # GPIOI6-FM_RESBTN_SLOT4_BMC_N
  # GPIOAC2-FM_RESBTN_SLOT1_BMC_N
  # GPIOAC3-FM_RESBTN_SLOT2_BMC_N
  devmem_clear_bit $(scu_addr AC) 2
  devmem_clear_bit $(scu_addr AC) 3
  gpio_export FM_RESBTN_SLOT3_BMC_N GPIOI4
  gpio_export FM_RESBTN_SLOT4_BMC_N GPIOI6
  gpio_export FM_RESBTN_SLOT1_BMC_N GPIOAC2
  gpio_export FM_RESBTN_SLOT2_BMC_N GPIOAC3
  gpio_set FM_RESBTN_SLOT1_BMC_N 0
  gpio_set FM_RESBTN_SLOT2_BMC_N 0
  gpio_set FM_RESBTN_SLOT3_BMC_N 0
  gpio_set FM_RESBTN_SLOT4_BMC_N 0
else
  echo "Is board id correct(id=$BOARD_ID)?"
fi

gpio_set BMC_READY_R 1
