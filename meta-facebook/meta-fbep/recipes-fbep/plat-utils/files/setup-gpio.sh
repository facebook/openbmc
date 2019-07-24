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

# To enable GPIOA
#devmem_clear_bit $(scu_addr 80) 0
#devmem_clear_bit $(scu_addr 80) 1
#devmem_clear_bit $(scu_addr 80) 3
#devmem_clear_bit $(scu_addr 80) 6
#devmem_clear_bit $(scu_addr 80) 7
#devmem_clear_bit $(scu_addr 90) 2

# OAM debug mode enable
gpio_export SCALE_DEBUG_EN_N_ASIC0 GPIOA7
gpio_export SCALE_DEBUG_EN_N_ASIC1 GPIOA6

# To enable GPIOB
#devmem_clear_scu70_bit 23
#devmem_clear_bit $(scu_addr 80) 13
#devmem_clear_bit $(scu_addr 80) 14

# PCIe switch GPIO (preserved)
gpio_export PAX2_INT GPIOB0
gpio_export PAX2_ALERT GPIOB1
gpio_export PAX3_INT GPIOB4
gpio_export PAX3_ALERT GPIOB5
gpio_export PAX0_INT2 GPIOB6
gpio_export PAX1_INT2 GPIOB7

# BMC ready
gpio_export BMC_READY_N GPIOB2
gpio_set BMC_READY_N 1

# System power good
gpio_export SYS_PWR_READY GPIOB3

# To enable GPIOD
#devmem_clear_scu70_bit 21
#devmem_clear_bit $(scu_addr 8c) 8
#devmem_clear_bit $(scu_addr 8c) 9
#devmem_clear_bit $(scu_addr 8c) 10
#devmem_clear_bit $(scu_addr 8c) 11
#devmem_clear_bit $(scu_addr 90) 1

# To enable GPIOE
#devmem_clear_scu70_bit 22
#devmem_clear_bit $(scu_addr 80) 16
#devmem_clear_bit $(scu_addr 80) 17
#devmem_clear_bit $(scu_addr 80) 18
#devmem_clear_bit $(scu_addr 80) 19
#devmem_clear_bit $(scu_addr 80) 20
#devmem_clear_bit $(scu_addr 80) 21
#devmem_clear_bit $(scu_addr 80) 22
#devmem_clear_bit $(scu_addr 80) 23
#devmem_clear_bit $(scu_addr 8c) 12
#devmem_clear_bit $(scu_addr 8c) 13
#devmem_clear_bit $(scu_addr 8c) 14
#devmem_clear_bit $(scu_addr 8c) 15

# BMC power button (input)
gpio_export BMC_PWR_BTN_IN_N GPIOE0

# BMC power cycle (output)
gpio_export BMC_IPMI_PWR_ON GPIOE1

# ASIC warm reset
gpio_export WARMRST_BMC_N_ASIC0 GPIOE7
gpio_export WARMRST_BMC_N_ASIC1 GPIOE6
gpio_export WARMRST_BMC_N_ASIC2 GPIOE5
gpio_export WARMRST_BMC_N_ASIC3 GPIOE4
gpio_export WARMRST_BMC_N_ASIC4 GPIOE3
gpio_export WARMRST_BMC_N_ASIC5 GPIOE2

# To enable GPIOF
#devmem_clear_bit $(scu_addr 80) 24
#devmem_clear_bit $(scu_addr 80) 25
#devmem_clear_bit $(scu_addr 80) 26
#devmem_clear_bit $(scu_addr 80) 27
#devmem_clear_bit $(scu_addr 80) 28
#devmem_clear_bit $(scu_addr 80) 29
#devmem_clear_bit $(scu_addr 80) 30
#devmem_clear_bit $(scu_addr 80) 31
#devmem_clear_bit $(scu_addr 90) 30

# POST LED 
gpio_export LED_POSTCODE_0 GPIOF0
gpio_set LED_POSTCODE_0 1
gpio_export LED_POSTCODE_1 GPIOF1
gpio_set LED_POSTCODE_1 1
gpio_export LED_POSTCODE_2 GPIOF2
gpio_set LED_POSTCODE_2 1
gpio_export LED_POSTCODE_3 GPIOF3
gpio_set LED_POSTCODE_3 1
gpio_export LED_POSTCODE_4 GPIOF4
gpio_set LED_POSTCODE_4 1
gpio_export LED_POSTCODE_5 GPIOF5
gpio_set LED_POSTCODE_5 1
gpio_export LED_POSTCODE_6 GPIOF6
gpio_set LED_POSTCODE_6 1
gpio_export LED_POSTCODE_7 GPIOF7
gpio_set LED_POSTCODE_7 1

# To enable GPIOG
#devmem_clear_bit $(scu_addr 84) 0
#devmem_clear_bit $(scu_addr 84) 1
#devmem_clear_bit $(scu_addr 84) 2
#devmem_clear_bit $(scu_addr 84) 3
#devmem_clear_bit $(scu_addr 84) 4
#devmem_clear_bit $(scu_addr 84) 5
#devmem_clear_bit $(scu_addr 84) 6
#devmem_clear_bit $(scu_addr 84) 7
#devmem_clear_bit $(scu_addr 94) 12

# TPM presence
gpio_export FM_BMC_TPM_PRES_N GPIOG1

# BMC heartbeat
gpio_export BMC_HEARTBEAT_N GPIOG2

# OAM test pin (defined by OAM)
gpio_export BMC_OAM_TEST7 GPIOG3
gpio_export BMC_OAM_TEST8 GPIOG7

# Board revision ID
gpio_export REV_ID0 GPIOG4
gpio_export REV_ID1 GPIOG5
gpio_export REV_ID2 GPIOG6

# To enable GPIOH
#devmem_clear_bit $(scu_addr 90) 7
#devmem_clear_bit $(scu_addr 94) 5
#devmem_clear_bit $(scu_addr 94) 6
#devmem_clear_bit $(scu_addr 94) 7

# ASIC presence
gpio_export PRSNT0_N_ASIC7 GPIOH0
gpio_export PRSNT0_N_ASIC6 GPIOH1
gpio_export PRSNT0_N_ASIC5 GPIOH2
gpio_export PRSNT0_N_ASIC4 GPIOH3
gpio_export PRSNT0_N_ASIC3 GPIOH4
gpio_export PRSNT0_N_ASIC2 GPIOH5
gpio_export PRSNT0_N_ASIC1 GPIOH6
gpio_export PRSNT0_N_ASIC0 GPIOH7

# To enable GPIOI
#devmem_clear_scu70_bit 13

# OAM debug pin (defined by OAM)
gpio_export BMC_OAM_TEST13 GPIOI0
gpio_export BMC_OAM_TEST14 GPIOI1

# OAM debug pin (defined by OAM)
gpio_export BMC_OAM_TEST3 GPIOJ0
gpio_export BMC_OAM_TEST4 GPIOJ1
gpio_export BMC_OAM_TEST5 GPIOJ2
gpio_export BMC_OAM_TEST6 GPIOJ3
gpio_export BMC_OAM_TEST0 GPIOJ6
gpio_export BMC_OAM_TEST1 GPIOJ7

# OAM debug mode enable
gpio_export SCALE_DEBUG_EN_N_ASIC5 GPIOJ4
gpio_export SCALE_DEBUG_EN_N_ASIC4 GPIOJ5

# To enable GPIOL
#devmem_clear_bit $(scu_addr 84) 16
#devmem_clear_bit $(scu_addr 84) 17
#devmem_clear_bit $(scu_addr 84) 18
#devmem_clear_bit $(scu_addr 84) 19
#devmem_clear_bit $(scu_addr 84) 20
#devmem_clear_bit $(scu_addr 84) 21
#devmem_clear_bit $(scu_addr 90) 5

# Select clock source from MB
# 0:0 = MB#1
# 1:0 = MB#2
# 0:1 = MB#3
# 1:1 = MB#4
gpio_export SEL0_CLK_MUX GPIOL0
gpio_export SEL1_CLK_MUX GPIOL1

# Output control for clock source from MB
# 0 = enable, 1 = high-impedance state
gpio_export OEA_CLK_MUX_N GPIOL2
gpio_set OEA_CLK_MUX_N 0
gpio_export OEB_CLK_MUX_N GPIOL3

# ASIC warm reset
gpio_export WARMRST_BMC_N_ASIC7 GPIOL4
gpio_export WARMRST_BMC_N_ASIC6 GPIOL5

# To enable GPIOM
#devmem_clear_bit $(scu_addr 84) 24
#devmem_clear_bit $(scu_addr 84) 25
#devmem_clear_bit $(scu_addr 84) 26
#devmem_clear_bit $(scu_addr 84) 27
#devmem_clear_bit $(scu_addr 84) 28
#devmem_clear_bit $(scu_addr 84) 29

# Slave i2c alert (defined by OAM)
gpio_export SMB_ALERT_ASIC01 GPIOM2
gpio_export SMB_ALERT_ASIC23 GPIOM3
gpio_export SMB_ALERT_ASIC45 GPIOM4
gpio_export SMB_ALERT_ASIC67 GPIOM5

# PCIe switch GPIO (preserved)
gpio_export PAX2_INT2 GPIOM0
gpio_export PAX3_INT2 GPIOM6

# SLED ID LED
gpio_export PWR_ID_LED_N GPIOM1

# RTC interrupt (PCF85263AT)
gpio_export BMC_RTC_INT GPIOM7

# To enable GPION
#devmem_clear_bit $(scu_addr 88) 2
#devmem_clear_bit $(scu_addr 88) 3
#devmem_clear_bit $(scu_addr 88) 4
#devmem_clear_bit $(scu_addr 88) 5
#devmem_clear_bit $(scu_addr 88) 6
#devmem_clear_bit $(scu_addr 88) 7

# OAM debug pin (defined by OAM)
gpio_export BMC_OAM_TEST2 GPION4

# Board ID
gpio_export BOARD_ID0 GPION5
gpio_export BOARD_ID1 GPION6
gpio_export BOARD_ID2 GPION7

# To enable GPIOO
#devmem_clear_bit $(scu_addr 88) 8
#devmem_clear_bit $(scu_addr 88) 9
#devmem_clear_bit $(scu_addr 88) 10
#devmem_clear_bit $(scu_addr 88) 11
#devmem_clear_bit $(scu_addr 88) 12
#devmem_clear_bit $(scu_addr 88) 13
#devmem_clear_bit $(scu_addr 88) 14
#devmem_clear_bit $(scu_addr 88) 15

# To enable GPIOP
#devmem_clear_bit $(scu_addr 88) 16
#devmem_clear_bit $(scu_addr 88) 17
#devmem_clear_bit $(scu_addr 88) 18
#devmem_clear_bit $(scu_addr 88) 19
#devmem_clear_bit $(scu_addr 88) 20
#devmem_clear_bit $(scu_addr 88) 21
#devmem_clear_bit $(scu_addr 88) 22
#devmem_clear_bit $(scu_addr 88) 23

# OAM debug pin (defined by OAM)
gpio_export BMC_OAM_TEST9 GPIOP0
gpio_export BMC_OAM_TEST10 GPIOP1
gpio_export BMC_OAM_TEST11 GPIOP2
gpio_export BMC_OAM_TEST12 GPIOP3

# ASIC presence
gpio_export PRSNT1_N_ASIC3 GPIOP4
gpio_export PRSNT1_N_ASIC2 GPIOP5
gpio_export PRSNT1_N_ASIC1 GPIOP6
gpio_export PRSNT1_N_ASIC0 GPIOP7

# To enable GPIOQ
#devmem_clear_bit $(scu_addr 2c) 1
#devmem_clear_bit $(scu_addr 2c) 29

# Battery voltage detect control
gpio_export BATTERY_DETECT GPIOQ6
gpio_set BATTERY_DETECT 1

# To enable GPIOR
#devmem_clear_bit $(scu_addr 88) 25
#devmem_clear_bit $(scu_addr 88) 26
#devmem_clear_bit $(scu_addr 88) 27
#devmem_clear_bit $(scu_addr 88) 28
#devmem_clear_bit $(scu_addr 88) 29
#devmem_clear_bit $(scu_addr 88) 30
#devmem_clear_bit $(scu_addr 88) 31

# OAM debug mode enable
gpio_export SCALE_DEBUG_EN_N_ASIC3 GPIOR6
gpio_export SCALE_DEBUG_EN_N_ASIC2 GPIOR7

# To enable GPIOS
#devmem_clear_bit $(scu_addr 8C) 0
#devmem_clear_bit $(scu_addr 8C) 1
#devmem_clear_bit $(scu_addr 8C) 2
#devmem_clear_bit $(scu_addr 8C) 3
#devmem_clear_bit $(scu_addr 8C) 4
#devmem_clear_bit $(scu_addr 8C) 5
#devmem_clear_bit $(scu_addr 8C) 6
#devmem_clear_bit $(scu_addr 8C) 7

# HSC throttle
gpio_export HSC1_THROT_N GPIOS0
gpio_export HSC2_THROT_N GPIOS1

# Set target OAM for test
gpio_export CPLD_MUX_ID3 GPIOS2

# PMBUS alert from HSC
gpio_export PMBUS_BMC_1_ALERT_N GPIOS3

# To enable GPIOT
#devmem_clear_bit $(scu_addr 48) 29
#devmem_clear_bit $(scu_addr 48) 30
#devmem_set_bit $(scu_addr a0) 0
#devmem_set_bit $(scu_addr a0) 4
#devmem_set_bit $(scu_addr a0) 5
#devmem_set_bit $(scu_addr a0) 6

# OAM debug mode enable
gpio_export SCALE_DEBUG_EN_N_ASIC6 GPIOT0
gpio_export SCALE_DEBUG_EN_N_ASIC7 GPIOT6

# To enable GPIOU
#devmem_set_bit $(scu_addr a0) 10
#devmem_set_bit $(scu_addr a0) 11
#devmem_set_bit $(scu_addr a0) 13

# OAM debug module presence
gpio_export DEBUG_PORT_PRSNT_N_ASIC7 GPIOU4
gpio_export DEBUG_PORT_PRSNT_N_ASIC6 GPIOU5
gpio_export DEBUG_PORT_PRSNT_N_ASIC5 GPIOU6
gpio_export DEBUG_PORT_PRSNT_N_ASIC4 GPIOU7

# To enable GPIOV
#devmem_set_bit $(scu_addr a0) 19

# OAM debug module presence
gpio_export DEBUG_PORT_PRSNT_N_ASIC3 GPIOV0
gpio_export DEBUG_PORT_PRSNT_N_ASIC2 GPIOV1
gpio_export DEBUG_PORT_PRSNT_N_ASIC1 GPIOV2
gpio_export DEBUG_PORT_PRSNT_N_ASIC0 GPIOV3

# PMBUS alert from HSC
gpio_export PMBUS_BMC_3_ALERT_N GPIOV5
gpio_export PMBUS_BMC_2_ALERT_N GPIOV6

# i2c alert from CPLD
gpio_export CPLD_SMB_ALERT_N GPIOV7

# To enable GPIOY
#devmem_clear_scu70_bit 19
#devmem_clear_bit $(scu_addr 94) 10
#devmem_clear_bit $(scu_addr 94) 11
#devmem_clear_bit $(scu_addr a4) 8
#devmem_clear_bit $(scu_addr a4) 9
#devmem_clear_bit $(scu_addr a4) 10
#devmem_clear_bit $(scu_addr a4) 11

# ASIC presence
gpio_export PRSNT1_N_ASIC7 GPIOY0
gpio_export PRSNT1_N_ASIC6 GPIOY1
gpio_export PRSNT1_N_ASIC5 GPIOY2
gpio_export PRSNT1_N_ASIC4 GPIOY3

# To enable GPIOZ
#devmem_clear_bit $(scu_addr 90) 31
#devmem_clear_bit $(scu_addr a4) 16
#devmem_clear_bit $(scu_addr a4) 17
#devmem_clear_bit $(scu_addr a4) 18
#devmem_clear_bit $(scu_addr a4) 19
#devmem_clear_bit $(scu_addr a4) 20
#devmem_clear_bit $(scu_addr a4) 21
#devmem_clear_bit $(scu_addr a4) 22
#devmem_clear_bit $(scu_addr a4) 23

# Select USB from MB
# 0:0 = MB#1
# 1:0 = MB#2
# 0:1 = MB#3
# 1:1 = MB#4
# PCH
gpio_export USB2_SEL0_U42 GPIOZ0
gpio_export USB2_SEL1_U42 GPIOZ1
# BMC
gpio_export USB2_SEL0_U43 GPIOZ2
gpio_export USB2_SEL1_U43 GPIOZ3

# To enable GPIOAA
#devmem_clear_bit $(scu_addr a4) 24
#devmem_clear_bit $(scu_addr a4) 25
#devmem_clear_bit $(scu_addr a4) 26
#devmem_clear_bit $(scu_addr a4) 27
#devmem_clear_bit $(scu_addr a4) 28
#devmem_clear_bit $(scu_addr a4) 29
#devmem_clear_bit $(scu_addr a4) 30
#devmem_clear_bit $(scu_addr a4) 31

# SPI flash select of PCIe switch
# 0: access by PCIe switch
# 1: access by BMC
gpio_export SEL_FLASH_PAX0 GPIOAA0
gpio_set SEL_FLASH_PAX0 0
gpio_export SEL_FLASH_PAX1 GPIOAA1
gpio_set SEL_FLASH_PAX1 0
gpio_export SEL_FLASH_PAX2 GPIOAA5
gpio_set SEL_FLASH_PAX2 0
gpio_export SEL_FLASH_PAX3 GPIOAA6
gpio_set SEL_FLASH_PAX3 0

# System log LED
gpio_export SYSTEM_LOG_LED GPIOAA2
gpio_set SYSTEM_LOG_LED 0

# Select USB from PCH/BMC of MB
# 0: BMC
# 1: PCH
gpio_export SEL_USB_MUX GPIOAA3

# SPI BMC write protection
gpio_export SPI_BMC_BT_WP0_N GPIOAA4

# To enable GPIOAB
#devmem_clear_bit $(scu_addr a8) 0
#devmem_clear_bit $(scu_addr a8) 1
#devmem_clear_bit $(scu_addr a8) 3

# TPM reset
gpio_export RST_PLTRST_BUF_N GPIOAB0

# Set target OAM for test
gpio_export CPLD_MUX_ID0 GPIOAB1
gpio_export CPLD_MUX_ID1 GPIOAB2
gpio_export CPLD_MUX_ID2 GPIOAB3

# To enable GPIOAC
#devmem_clear_bit $(scu_addr ac) 0
#devmem_clear_bit $(scu_addr ac) 1
#devmem_clear_bit $(scu_addr ac) 2
#devmem_clear_bit $(scu_addr ac) 3

# PCIe switch GPIO (preserved)
gpio_export PAX0_INT GPIOAC0
gpio_export PAX0_ALERT GPIOAC1
gpio_export PAX1_INT GPIOAC2
gpio_export PAX1_ALERT GPIOAC3
