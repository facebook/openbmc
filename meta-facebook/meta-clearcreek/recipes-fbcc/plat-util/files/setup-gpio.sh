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

# To enable GPIOAC
devmem_clear_bit $(scu_addr ac) 0
devmem_clear_bit $(scu_addr ac) 1
devmem_clear_bit $(scu_addr ac) 2
devmem_clear_bit $(scu_addr ac) 3
devmem_clear_bit $(scu_addr ac) 5
devmem_clear_bit $(scu_addr ac) 6
devmem_clear_bit $(scu_addr ac) 7

#setup BMC SKU ID
gpio_export SKU_ID1_PAX0 GPIOB6

gpio_export SKU_ID0_PAX0 GPIOAC0

gpio_export SKU_ID1_PAX1 GPIOB7

gpio_export SKU_ID0_PAX1 GPIOAC2


gpio_export SKU_ID1_PAX2 GPIOM0

gpio_export SKU_ID0_PAX2 GPIOB0

gpio_export SKU_ID1_PAX3 GPIOM6

gpio_export SKU_ID0_PAX3 GPIOB4

# PDB 12V POWER (output)
gpio_export BMC_IPMI_PWR_ON GPIOE1
gpio_set BMC_IPMI_PWR_ON 1

# BMC ready
gpio_export BMC_READY_N GPIOB2

# System power good
gpio_export SYS_PWR_READY GPIOB3

# BMC power button (input)
gpio_export BMC_PWR_BTN_IN_N GPIOE0

gpio_set BMC_READY_N 0

# Reserved
gpio_export BMC_GPIOA1 GPIOA1
gpio_export BMC_GPIOA2 GPIOA2
gpio_export BMC_GPIOA3 GPIOA3

# POST LED 
gpio_export LED_POSTCODE_0 GPIOF0
gpio_set LED_POSTCODE_0 0
gpio_export LED_POSTCODE_1 GPIOF1
gpio_set LED_POSTCODE_1 0
gpio_export LED_POSTCODE_2 GPIOF2
gpio_set LED_POSTCODE_2 0
gpio_export LED_POSTCODE_3 GPIOF3
gpio_set LED_POSTCODE_3 0
gpio_export LED_POSTCODE_4 GPIOF4
gpio_set LED_POSTCODE_4 0
gpio_export LED_POSTCODE_5 GPIOF5
gpio_set LED_POSTCODE_5 0
gpio_export LED_POSTCODE_6 GPIOF6
gpio_set LED_POSTCODE_6 0
gpio_export LED_POSTCODE_7 GPIOF7
gpio_set LED_POSTCODE_7 0

# Board revision ID
gpio_export REV_ID0 GPIOG4
gpio_export REV_ID1 GPIOG5
gpio_export REV_ID2 GPIOG6

# OCP3.0 NIC power good
gpio_export OCP_V3_0_NIC_POWER_GOOD GPIOH0
gpio_export OCP_V3_1_NIC_POWER_GOOD GPIOH1
gpio_export OCP_V3_2_NIC_POWER_GOOD GPIOH2
gpio_export OCP_V3_3_NIC_POWER_GOOD GPIOH3
gpio_export OCP_V3_4_NIC_POWER_GOOD GPIOH4
gpio_export OCP_V3_5_NIC_POWER_GOOD GPIOH5
gpio_export OCP_V3_6_NIC_POWER_GOOD GPIOH6
gpio_export OCP_V3_7_NIC_POWER_GOOD GPIOH7

# SEL FLASH PAX
gpio_export SEL_FLASH_PAX0 GPIOAA0
gpio_export SEL_FLASH_PAX1 GPIOAA1
gpio_export SEL_FLASH_PAX2 GPIOAA5
gpio_export SEL_FLASH_PAX3 GPIOAA6

gpio_export OCP_V3_3_PRSNTB_R_N GPIOAA2
gpio_export OCP_V3_4_PRSNTB_R_N GPIOAA3
gpio_export OCP_V3_5_PRSNTB_R_N GPIOAB3

gpio_export BMC_BOOT_RCVR_B0_PAX0 GPIOL0
gpio_set BMC_BOOT_RCVR_B0_PAX0 0
gpio_export BMC_BOOT_RCVR_B0_PAX1 GPIOL1
gpio_set BMC_BOOT_RCVR_B0_PAX1 0
gpio_export BMC_BOOT_RCVR_B0_PAX2 GPIOL2
gpio_set BMC_BOOT_RCVR_B0_PAX2 0
gpio_export BMC_BOOT_RCVR_B0_PAX3 GPIOL3
gpio_set BMC_BOOT_RCVR_B0_PAX3 0

# BSM EMMC PRSNT
gpio_export BSM_EMMC_PRSNT_R_N GPIOL4

gpio_export RST_BMC_SMB4_BUF_R_N GPIOL5

# NVME PRSNT
gpio_export NVME_0_1_PRSNTB_R_N GPIOL6
gpio_export NVME_2_3_PRSNTB_R_N GPIOL7

gpio_export SMB_MB_BMC_MUX_OE_R GPIOM3
gpio_set SMB_MB_BMC_MUX_OE_R 0
gpio_export SMB_MB_BMC_MUX_S0_R GPIOM4
gpio_set SMB_MB_BMC_MUX_S0_R 0
gpio_export SMB_MB_BMC_MUX_S1_R GPIOM5
gpio_set SMB_MB_BMC_MUX_S1_R 0

# BOARD ID
gpio_export BOARD_ID0 GPION5
gpio_export BOARD_ID1 GPION6
gpio_export BOARD_ID2 GPION7

gpio_export OCP_V3_6_PRSNTB_R_N GPIOS2
gpio_export OCP_V3_7_PRSNTB_R_N GPIOS3
gpio_export PD_GPIOS4 GPIOS4
gpio_set PD_GPIOS4 0
gpio_export PD_GPIOS5 GPIOS5
gpio_export PD_GPIOS6 GPIOS6
gpio_set PD_GPIOS6 0
gpio_export PD_GPIOS7 GPIOS7
gpio_set PD_GPIOS7 0

gpio_export OCP_V3_0_PRSNTB_R_N GPIOY3

gpio_export OCP_V3_1_PRSNTB_R_N GPIOZ2
gpio_export OCP_V3_2_PRSNTB_R_N GPIOZ3

gpio_export PU_GPIOZ4 GPIOZ4
gpio_set PU_GPIOZ4 0
gpio_export PU_GPIOZ5 GPIOZ5
gpio_set PU_GPIOZ5 0
gpio_export PU_GPIOZ6 GPIOZ6
gpio_set PU_GPIOZ6 0
gpio_export PU_GPIOZ7 GPIOZ7
gpio_set PU_GPIOZ7 1

gpio_export PD_GPIOT1 GPIOT1
gpio_set PD_GPIOT1 0
gpio_export NCSI_BMC_TX_EN GPIOT2
gpio_set NCSI_BMC_TX_EN 1
gpio_export NCSI_BMC_TXD0 GPIOT3
gpio_set NCSI_BMC_TXD0 0
gpio_export NCSI_BMC_TXD1 GPIOT4
gpio_set NCSI_BMC_TXD1 0
gpio_export PD_GPIOT5 GPIOT5
gpio_set PD_GPIOT5 0

gpio_export PD_GPIOT7 GPIOT7
gpio_set PD_GPIOT7 0
gpio_export NCSI_BMC_2_TX_EN GPIOU0
gpio_set NCSI_BMC_2_TX_EN 0
gpio_export NCSI_BMC_2_TXD0 GPIOU1
gpio_set NCSI_BMC_2_TXD0 0
gpio_export NCSI_BMC_2_TXD1 GPIOU2
gpio_set NCSI_BMC_2_TXD1 0
gpio_export PD_GPIOU3 GPIOU3
gpio_set PD_GPIOU3 0

gpio_export USB2_SEL0 GPIOZ0
gpio_set USB2_SEL0 0
gpio_export USB2_SEL1 GPIOZ1
gpio_set USB2_SEL1 0

gpio_export CPLD_BMC_GPIO_R_00 GPIOAC1
gpio_set CPLD_BMC_GPIO_R_00 0

gpio_export RST_FIO_IOEXP_R_N GPIOY1
gpio_set RST_FIO_IOEXP_R_N 1

gpio_export CPLD_BMC_GPIO_R_01 GPIOAC3
