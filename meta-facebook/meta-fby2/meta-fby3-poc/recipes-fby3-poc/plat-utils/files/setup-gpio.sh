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

# Set up BMC ready pin
# GPIOA0-BMC_READY_R (pin number 0)
devmem_clear_bit $(scu_addr 80) 0
gpio_set A0 0

# Set up hotswap alert pin
# GPIOA1-SMB_HOTSWAP_ALERT_N (pin number 1)
devmem_clear_bit $(scu_addr 80) 1
gpio_export A1

# Set up hsc fault pin
# GPIOA2-FM_HSC_FAULT_N (pin number 2)
devmem_clear_bit $(scu_addr 80) 15
gpio_export A2

# Set up an alert pin for a server board
# GPIOB0-SMB_SLOT1_CPLD_BIC_ALERT_N (pin number 8)
# GPIOB1-SMB_SLOT2_CPLD_BIC_ALERT_N (pin number 9)
# GPIOB2-SMB_SLOT3_CPLD_BIC_ALERT_N (pin number 10)
# GPIOB3-SMB_SLOT4_CPLD_BIC_ALERT_N (pin number 11)
gpio_export B0
gpio_export B1
gpio_export B2
gpio_export B3

# Set up a present pin for a server board
# GPIOB4-PRSNT_MB_SLOT1_BB_N (pin number 12)
# GPIOB5-PRSNT_MB_SLOT2_BB_N (pin number 13)
# GPIOB6-PRSNT_MB_SLOT3_BB_N (pin number 14)
# GPIOB7-PRSNT_MB_SLOT4_BB_N (pin number 15)
devmem_clear_bit $(scu_addr 80) 13
devmem_clear_bit $(scu_addr 80) 14
gpio_export B4
gpio_export B5
gpio_export B6
gpio_export B7

# Set up a EMMC SD pin
# GPIOD0-EMMC_SD2CLK_R1  (function pin)
# GPIOD1-EMMC_SD2CMD_R1  (function pin)
# GPIOD2-EMMC_SD2DAT0_R1 (function pin)
# GPIOD3-EMMC_SD2DAT0_R1 (function pin)
# GPIOD4-EMMC_SD2DAT0_R1 (function pin)
# GPIOD5-EMMC_SD2DAT0_R1 (function pin)
# GPIOD6-SD2DET_N (function pin)
# GPIOD7-SD2WPT_N (function pin)
devmem_set_bit $(scu_addr 90) 1

# Set up a pin for detecting NIC Adapter Card type
# GPIOE0-ADAPTER_CARD_TYPE_1 (pin number 32)
# GPIOE1-ADAPTER_CARD_TYPE_2 (pin number 33)
# GPIOE2-ADAPTER_CARD_TYPE_3 (pin number 34)
devmem_clear_bit $(scu_addr 80) 16
devmem_clear_bit $(scu_addr 80) 17
devmem_clear_bit $(scu_addr 80) 18
devmem_clear_bit $(scu_addr 8C) 12
devmem_clear_bit $(scu_addr 8C) 13
gpio_export E0
gpio_export E1
gpio_export E2

# Set up a pin to check if the sled AC cycle is occured
# GPIOE3-ADAPTER_BUTTON_BMC_N (pin number 35)
devmem_clear_bit $(scu_addr 80) 19
gpio_export E3

# Set up a pin for the alert from temperature
# GPIOE4-SMB_TEMP_ALERT_N (pin number 36)
devmem_clear_bit $(scu_addr 80) 20
devmem_clear_bit $(scu_addr 8C) 14
gpio_export E4

# Set up a pin for enabling hsc
# GPIOF0-HSC_CPLD_SLOT1_EN_R (pin number 40)
# GPIOF1-HSC_CPLD_SLOT2_EN_R (pin number 41)
# GPIOF2-HSC_CPLD_SLOT3_EN_R (pin number 42)
# GPIOF3-HSC_CPLD_SLOT4_EN_R (pin number 43)
devmem_clear_bit $(scu_addr 80) 24
devmem_clear_bit $(scu_addr 80) 25
devmem_clear_bit $(scu_addr 80) 26
devmem_clear_bit $(scu_addr 80) 27
devmem_clear_bit $(scu_addr 90) 30
gpio_set F0 0
gpio_set F1 0
gpio_set F2 0
gpio_set F3 0

# Set up the primary pin for reseting NIC
# GPIOF4-SMB_RST_PRIMARY_CPLD_N_R (pin number 44)
devmem_clear_bit $(scu_addr 80) 28
gpio_set F4 1

# Set up to read the board revision pins
# GPIOG0-BOARD_REV_ID0 (pin number 48)
# GPIOG1-BOARD_REV_ID0 (pin number 49)
# GPIOG2-BOARD_REV_ID0 (pin number 50)
devmem_clear_bit $(scu_addr 84) 0
devmem_clear_bit $(scu_addr 84) 1
devmem_clear_bit $(scu_addr 84) 2
gpio_export G0
gpio_export G1
gpio_export G2

# Set up the secondary pin for reseting NIC
# GPIOG3-SMB_RST_SECONDARY_CPLD_N_R (pin number 51)
devmem_clear_bit $(scu_addr 84) 3
gpio_set G3 1

# Set up a pin for NIC/CPLD UART
# GPIOH6-UART_NIC_CPLD_TX_R (function pin)
# GPIOH7-UART_NIC_CPLD_RX (function pin)
devmem_clear_bit $(scu_addr 90) 6
devmem_set_bit $(scu_addr 90) 7


# Set up a pin for enabling the variable resistor 
# GPIOG4-SLOT1_HSC_SET_CPLD_EN_R (pin number 52)
# GPIOG5-SLOT2_HSC_SET_CPLD_EN_R (pin number 53)
# GPIOG6-SLOT3_HSC_SET_CPLD_EN_R (pin number 54)
# GPIOG7-SLOT4_HSC_SET_CPLD_EN_R (pin number 55)
devmem_clear_bit $(scu_addr 94) 12
devmem_clear_bit $(scu_addr 84) 4
devmem_clear_bit $(scu_addr 84) 5
devmem_clear_bit $(scu_addr 84) 6
devmem_clear_bit $(scu_addr 84) 7
gpio_set G4 0
gpio_set G5 0
gpio_set G6 0
gpio_set G7 0


# Set up a pin for power good
# GPIOJ0-PWROK_STBY_BMC_SLOT1 (pin number 72)
# GPIOJ1-PWROK_STBY_BMC_SLOT2 (pin number 73)
# GPIOJ2-PWROK_STBY_BMC_SLOT3 (pin number 74)
# GPIOJ3-PWROK_STBY_BMC_SLOT4 (pin number 75)
devmem_clear_bit $(scu_addr 84) 8
devmem_clear_bit $(scu_addr 84) 9
devmem_clear_bit $(scu_addr 84) 10
devmem_clear_bit $(scu_addr 84) 11
gpio_export J0
gpio_export J1
gpio_export J2
gpio_export J3

# Set up a pin for receiving the PCIE reset signal from a slot
# GPIOJ4-RST_PCIE_RESET_SLOT1_N (pin number 76)
# GPIOJ5-RST_PCIE_RESET_SLOT2_N (pin number 77)
# GPIOJ6-RST_PCIE_RESET_SLOT3_N (pin number 78)
# GPIOJ7-RST_PCIE_RESET_SLOT4_N (pin number 79)
devmem_clear_bit $(scu_addr 84) 12
devmem_clear_bit $(scu_addr 84) 13
devmem_clear_bit $(scu_addr 84) 14
devmem_clear_bit $(scu_addr 84) 15
devmem_clear_bit $(scu_addr 94) 8
devmem_clear_bit $(scu_addr 94) 9
gpio_export J4
gpio_export J5
gpio_export J6
gpio_export J7

# Set up a pin for controlling server board power status
# GPIOL0-AC_ON_OFF_BTN_SLOT1_N (pin number 88)
# GPIOL1-AC_ON_OFF_BTN_SLOT2_N (pin number 89)
# GPIOL2-AC_ON_OFF_BTN_SLOT3_N (pin number 90)
# GPIOL3-AC_ON_OFF_BTN_SLOT4_N (pin number 91)
devmem_clear_bit $(scu_addr 84) 16
devmem_clear_bit $(scu_addr 84) 17
devmem_clear_bit $(scu_addr 84) 18
devmem_clear_bit $(scu_addr 84) 19
gpio_set L0 1
gpio_set L1 1
gpio_set L2 1
gpio_set L3 1

# Set up a pin to check if HSC Fast Prochot is enabled
# GPIOL4-FAST_PROCHOT_N (pin number 92)
devmem_clear_bit $(scu_addr 84) 20
gpio_export L4

# Set up a pin for enabling the USB power switch by BMC 
# GPIOL5-USB_CPLD_EN_N_R (pin number 93)
devmem_clear_bit $(scu_addr 84) 21
gpio_set L5 0

# Set up a pin for receiving the HSC fault from a slot
# GPIOM0-HS0_FAULT_SLOT1_N (pin number 96)
# GPIOM1-HS0_FAULT_SLOT2_N (pin number 97)
# GPIOM2-HS0_FAULT_SLOT3_N (pin number 98)
# GPIOM3-HS0_FAULT_SLOT4_N (pin number 99)
devmem_clear_bit $(scu_addr 84) 24
devmem_clear_bit $(scu_addr 84) 25
devmem_clear_bit $(scu_addr 84) 26
devmem_clear_bit $(scu_addr 84) 27
gpio_export M0
gpio_export M1
gpio_export M2
gpio_export M3

# Set up a pin to read NIC Card ID
# GPIOM4-FM_BB_CPLD_SLOT_ID0_R (pin number 100)
# GPIOM5-FM_BB_CPLD_SLOT_ID1_R (pin number 101)
devmem_clear_bit $(scu_addr 84) 28
devmem_clear_bit $(scu_addr 84) 29
gpio_export M4
gpio_export M5

# Set up a pin for enabling USB OC protect
# GPIOQ6-USB_OC_N (pin number 134)
devmem_clear_bit $(scu_addr 2C) 1
gpio_export Q6

# Set up a pin to detect TPM present
# GPIOQ7-FM_BMC_TPM_PRSNT_N (pin number 135)
devmem_clear_bit $(scu_addr 2C) 29
gpio_export Q7

# Set up a pin to check NIC Card power good
# GPIOAB0-PWRGD_NIC_BMC (pin number 216)
devmem_clear_bit $(scu_addr A8) 0
gpio_export AB0

# Set up a pin to detect OCP debug card present
# GPIOAB1-OCP_DEBUG_PRSNT_N (pin number 217)
devmem_clear_bit $(scu_addr A8) 1
gpio_export AB1

# Set up a pin to clean CMOS on a slot
# GPIOAC0-CMOS_CLEAR_SLOT1_R (pin number 220)
# GPIOAC1-CMOS_CLEAR_SLOT2_R (pin number 221)
# GPIOAC2-CMOS_CLEAR_SLOT3_R (pin number 222)
# GPIOAC3-CMOS_CLEAR_SLOT4_R (pin number 223)
devmem_clear_scu70_bit 25
devmem_clear_bit $(scu_addr AC) 0
devmem_clear_bit $(scu_addr AC) 1
devmem_clear_bit $(scu_addr AC) 2
devmem_clear_bit $(scu_addr AC) 3
gpio_export AC0
gpio_export AC1
gpio_export AC2
gpio_export AC3
