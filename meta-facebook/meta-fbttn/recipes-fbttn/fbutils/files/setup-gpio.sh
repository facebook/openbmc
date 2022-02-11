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
# exported in other files.  We should probably move some more of the
# definitions to this file at some point.

. /usr/local/fbpackages/utils/ast-functions

# This is a workaround for MAC1LINK
devmem 0x1e660050 w 0xA954F

# BMC_TO_EXP_RESET: A2 (2)
# To use GPIOA2, SCU80[2], and SCU80[15] must be 0
devmem_clear_bit $(scu_addr 80) 2
devmem_clear_bit $(scu_addr 80) 15

gpio_export BMC_TO_EXP_RESET GPIOA2
# set GPIOA2 WDT reset tolerance
gpio_tolerance_fun BMC_TO_EXP_RESET GPIOA2

# SYS_PWR_LED: A3 (3)
# To use GPIOA3, SCU80[3] must be 0
devmem_clear_bit $(scu_addr 80) 3

gpio_export SYS_PWR_LED GPIOA3
gpio_set SYS_PWR_LED 1

# PWRBTN_OUT_N: D2 (26)
# To use GPIOD2, SCU90[1], SCU8C[9], and SCU70[21] must be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8C) 9
devmem_clear_bit $(scu_addr 70) 21

gpio_export PWRBTN_OUT_N GPIOD2

# COMP_PWR_BTN_N: D3 (27)
# To use GPIOD3, SCU90[1], SCU8C[9], and SCU70[21] must be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8C) 9
devmem_clear_bit $(scu_addr 70) 21

gpio_export COMP_PWR_BTN_N GPIOD3
gpio_set COMP_PWR_BTN_N 1
# set GPIOD3 WDT reset tolerance
gpio_tolerance_fun COMP_PWR_BTN_N GPIOD3

# RSTBTN_OUT_N: D4 (28)
# To use GPIOD4, SCU90[1], SCU8C[10], and SCU70[21] must be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8C) 10
devmem_clear_bit $(scu_addr 70) 21

gpio_export RSTBTN_OUT_N GPIOD4

# SYS_RESET_N_OUT: D5 (29)
# To use GPIOD5, SCU90[1], SCU8C[10], and SCU70[21] must be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8C) 10
devmem_clear_bit $(scu_addr 70) 21

gpio_export SYS_RESET_N_OUT GPIOD5
gpio_set SYS_RESET_N_OUT 1
# set GPIOD5 WDT reset tolerance
gpio_tolerance_fun SYS_RESET_N_OUT GPIOD5

# M2_1_PRESENT_N: E0 (32)
# To use GPIOE0, SCU80[16], SCU8C[12], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 16
devmem_clear_bit $(scu_addr 8C) 12
devmem_clear_bit $(scu_addr 70) 22

gpio_export M2_1_PRESENT_N GPIOE0

# M2_2_PRESENT_N: E1 (33)
# To use GPIOE1, SCU80[17], SCU8C[12], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 17
devmem_clear_bit $(scu_addr 8C) 12
devmem_clear_bit $(scu_addr 70) 22

gpio_export M2_2_PRESENT_N GPIOE1

# M2_1_FAULT_N: E2 (34)
# To use GPIOE2, SCU80[18], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 18
devmem_clear_bit $(scu_addr 8C) 13
devmem_clear_bit $(scu_addr 70) 22

gpio_export M2_1_FAULT_N GPIOE2

# M2_2_FAULT_N: E3 (35)
# To use GPIOE3, SCU80[19], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 19
devmem_clear_bit $(scu_addr 8C) 13
devmem_clear_bit $(scu_addr 70) 22

gpio_export M2_2_FAULT_N GPIOE3

# BMC_EXT1_LED_B: E4 (36)
# To use GPIOE4, SCU80[20], SCU8C[14], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 20
devmem_clear_bit $(scu_addr 8C) 14
devmem_clear_bit $(scu_addr 70) 22

gpio_export BMC_EXT1_LED_B GPIOE4
gpio_set BMC_EXT1_LED_B 0

# BMC_EXT1_LED_Y: E5 (37)
# To use GPIOE5, SCU80[21], SCU8C[14], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 21
devmem_clear_bit $(scu_addr 8C) 14
devmem_clear_bit $(scu_addr 70) 22

gpio_export BMC_EXT1_LED_Y GPIOE5
gpio_set BMC_EXT1_LED_Y 1

# BMC_EXT2_LED_B: E6 (38)
# To use GPIOE6, SCU80[22], SCU8C[15], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 22
devmem_clear_bit $(scu_addr 8C) 15
devmem_clear_bit $(scu_addr 70) 22

gpio_export BMC_EXT2_LED_B GPIOE6
gpio_set BMC_EXT2_LED_B 0

# BMC_EXT2_LED_Y: E7 (39)
# To use GPIOE7, SCU80[23], SCU8C[15], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 23
devmem_clear_bit $(scu_addr 8C) 15
devmem_clear_bit $(scu_addr 70) 22

gpio_export BMC_EXT2_LED_Y GPIOE7
gpio_set BMC_EXT2_LED_Y 1

# Because SCC power sequence is controlled by HW on EVT.
# So we just configure SCC PWR pins to GPIO and does not export their GPIO node.
# SCC PWR pins: GPIOF0, GPIOF1, and GPIOF4
# SCC_LOC_FULL_PWR_EN: F0 (40), PS
# To use GPIOF0, LHCR[0], SCU90[30], and SCU80[24] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 24

# SCC_RMT_FULL_PWR_EN: F1 (41)
# To use GPIOF1, LHCR[0], SCU90[30], and SCU80[25] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 25

gpio_export SCC_RMT_FULL_PWR_EN GPIOF1

# EXP_SPARE_0: F2 (42), EXP_SPARE_1: F3 (43)
# To use GPIOF2, LHCR[0], SCU90[30], and SCU80[26] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 26
# To use GPIOF3, LHCR[0], SCU90[30], and SCU80[27] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 27

gpio_export EXP_SPARE_0 GPIOF2
gpio_export EXP_SPARE_1 GPIOF3


# SCC_STBY_PWR_EN: F4 (44), PS
# To use GPIOF4, LHCR[0], SCU90[30], and SCU80[28] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 28

# DPB_MISC_ALERT: F5 (45)
# To use GPIOF5, LHCR[0], SCU90[30], and SCU80[29] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 29

gpio_export DPB_MISC_ALERT GPIOF5

# P12V_A_PGOOD: F6 (46)
# To use GPIOF6, LHCR[0], and SCU80[30] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 80) 30

gpio_export P12V_A_PGOOD GPIOF6

# SCC_RMT_TYPE_0: F7 (47)
# To use GPIOF7, LHCR[0], SCU90[30], and SCU80[31] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 31

gpio_export SCC_RMT_TYPE_0 GPIOF7

# SLOTID_0: G0 (48), SLOTID_1: G1 (49)
# To use GPIOG0, SCU84[0] must be 0
devmem_clear_bit $(scu_addr 84) 0
# To use GPIOG1, SCU84[1] must be 0
devmem_clear_bit $(scu_addr 84) 1

gpio_export SLOTID_0 GPIOG0
gpio_export SLOTID_1 GPIOG1

# COMP_SPARE_0: G2 (50), COMP_SPARE_1: G3 (51))
# To use GPIOG2, SCU84[2] must be 0
devmem_clear_bit $(scu_addr 84) 2
# To use GPIOG3, SCU84[3] must be 0
devmem_clear_bit $(scu_addr 84) 3

gpio_export COMP_SPARE_0 GPIOG2
gpio_export COMP_SPARE_1 GPIOG3


# BIC_READY_N: G7 (55)
# To use GPIOG7, SCU94[12], and SCU84[7] must be 0
devmem_clear_bit $(scu_addr 94) 12
devmem_clear_bit $(scu_addr 84) 7

gpio_export BIC_READY_N GPIOG7

# LED_POSTCODE_0: H0 (56)
# To use GPIOH0, SCU94[5], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 5
devmem_clear_bit $(scu_addr 90) 7

gpio_export LED_POSTCODE_0 GPIOH0
gpio_set LED_POSTCODE_0 0

# LED_POSTCODE_1: H1 (57)
# To use GPIOH1, SCU94[5], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 5
devmem_clear_bit $(scu_addr 90) 7

gpio_export LED_POSTCODE_1 GPIOH1
gpio_set LED_POSTCODE_1 0

# LED_POSTCODE_2: H2 (58)
# To use GPIOH2, SCU94[6], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 6
devmem_clear_bit $(scu_addr 90) 7

gpio_export LED_POSTCODE_2 GPIOH2
gpio_set LED_POSTCODE_2 0

# LED_POSTCODE_3: H3 (59)
# To use GPIOH3, SCU94[6], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 6
devmem_clear_bit $(scu_addr 90) 7

gpio_export LED_POSTCODE_3 GPIOH3
gpio_set LED_POSTCODE_3 0

# LED_POSTCODE_4: H4 (60)
# To use GPIOH4, SCU94[7], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 7
devmem_clear_bit $(scu_addr 90) 7

gpio_export LED_POSTCODE_4 GPIOH4
gpio_set LED_POSTCODE_4 0

# LED_POSTCODE_5: H5 (61)
# To use GPIOH5, SCU94[7], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 7
devmem_clear_bit $(scu_addr 90) 7

gpio_export LED_POSTCODE_5 GPIOH5
gpio_set LED_POSTCODE_5 0

# LED_POSTCODE_6: H6 (62)
# To use GPIOH6, SCU90[7] must be 0
devmem_clear_bit $(scu_addr 90) 7

gpio_export LED_POSTCODE_6 GPIOH6
gpio_set LED_POSTCODE_6 0

# LED_POSTCODE_7: H7 (63)
# To use GPIOH7, SCU90[7] must be 0
devmem_clear_bit $(scu_addr 90) 7

gpio_export LED_POSTCODE_7 GPIOH7
gpio_set LED_POSTCODE_7 0

# BOARD_REV_0: J0 (72), BOARD_REV_1: J1 (73), BOARD_REV_2: J2 (74)
# To use GPIOJ0, SCU84[8] must be 0
devmem_clear_bit $(scu_addr 84) 8
# To use GPIOJ1, SCU84[9] must be 0
devmem_clear_bit $(scu_addr 84) 9
# To use GPIOJ2, SCU84[10] must be 0
devmem_clear_bit $(scu_addr 84) 10

gpio_export BOARD_REV_0 GPIOJ0
gpio_export BOARD_REV_1 GPIOJ1
gpio_export BOARD_REV_2 GPIOJ2


# PCIE_WAKE_N: J3 (75)
# To use GPIOJ3, SCU84[11] must be 0
devmem_clear_bit $(scu_addr 84) 11

gpio_export PCIE_WAKE_N GPIOJ3

# IOM_TYPE0: J4 (76)
# To use GPIOJ4, SCU84[12] must be 0
# To use GPIOJ4, SCU94[8] must be 0
devmem_clear_bit $(scu_addr 84) 12
devmem_clear_bit $(scu_addr 94) 8

gpio_export IOM_TYPE0 GPIOJ4

# IOM_TYPE1: J5 (77)
# To use GPIOJ5, SCU84[13] must be 0
# To use GPIOJ5, SCU94[8] must be 0
devmem_clear_bit $(scu_addr 84) 13
devmem_clear_bit $(scu_addr 94) 8

gpio_export IOM_TYPE1 GPIOJ5

# IOM_TYPE2: J6 (78)
# To use GPIOJ6, SCU84[14] must be 0
# To use GPIOJ6, SCU94[9] must be 0
devmem_clear_bit $(scu_addr 84) 14
devmem_clear_bit $(scu_addr 94) 9

gpio_export IOM_TYPE2 GPIOJ6

# IOM_TYPE3: J7 (79)
# To use GPIOJ7, SCU84[15] must be 0
# To use GPIOJ7, SCU94[9] must be 0
devmem_clear_bit $(scu_addr 84) 15
devmem_clear_bit $(scu_addr 94) 9

gpio_export IOM_TYPE3 GPIOJ7

# BMC_RMT_HEARTBEAT(TACH): O0 (112)
# To use GPIOO0, SCU88[8] must be 0
devmem_clear_bit $(scu_addr 88) 8

gpio_export BMC_RMT_HEARTBEAT GPIOO0

# BMC_LOC_HEARTBEAT: O1 (113)
# To use GPIOO1, SCU88[9] must be 0
devmem_clear_bit $(scu_addr 88) 9

gpio_export BMC_LOC_HEARTBEAT GPIOO1
gpio_set BMC_LOC_HEARTBEAT 1

# FLA0_WP_N: O2 (114)
# To use GPIOO1, SCU88[10] must be 0
devmem_clear_bit $(scu_addr 88) 10

gpio_export FLA0_WP_N GPIOO2

# ENCL_FAULT_LED: O3 (115)
# To use GPIOO1, SCU88[11] must be 0
devmem_clear_bit $(scu_addr 88) 11

gpio_export ENCL_FAULT_LED GPIOO3
gpio_set ENCL_FAULT_LED 0

# SCC_LOC_HEARTBEAT: O4 (116)
# To use GPIOO4, SCU88[12] must be 0
devmem_clear_bit $(scu_addr 88) 12

gpio_export SCC_LOC_HEARTBEAT GPIOO4

# SCC_RMT_HEARTBEAT: O5 (117)
# To use GPIOO5, SCU88[13] must be 0
devmem_clear_bit $(scu_addr 88) 13

gpio_export SCC_RMT_HEARTBEAT GPIOO5

# COMP_POWER_FAIL_N: O6 (118)
# To use GPIOO6, SCU88[14] must be 0
devmem_clear_bit $(scu_addr 88) 14

gpio_export COMP_POWER_FAIL_N GPIOO6

# COMP_PWR_EN: O7 (119), PS
# To use GPIOO7, SCU88[15] must be 0
devmem_clear_bit $(scu_addr 88) 15

gpio_export COMP_PWR_EN GPIOO7
# set GPIOO7 WDT reset tolerance
gpio_tolerance_fun COMP_PWR_EN GPIOO7

# Disable GPIOP internal pull down
devmem_set_bit $(scu_addr 8C) 31

# DEBUG_GPIO_BMC_6: P0 (120)
# To use GPIOP0, SCU88[16] must be 0
devmem_clear_bit $(scu_addr 88) 16

gpio_export DEBUG_GPIO_BMC_6 GPIOP0
gpio_set DEBUG_GPIO_BMC_6 1
# set GPIOP0 WDT reset tolerance
gpio_tolerance_fun DEBUG_GPIO_BMC_6 GPIOP0

# COMP_FAST_THROTTLE_N: P2 (122)
# To use GPIOP2, SCU88[18] must be 0
devmem_clear_bit $(scu_addr 88) 18

# TODO: Disable the CPU Throttle PIN to fix the leakage issue from BMC to ML
gpio_export COMP_FAST_THROTTLE_N GPIOP2

# DEBUG_PWR_BTN_N: P3 (123)
# To use GPIOP3, SCU88[19] must be 0
devmem_clear_bit $(scu_addr 88) 19

gpio_export DEBUG_PWR_BTN_N GPIOP3

# DEBUG_GPIO_BMC_2: P4 (124)
# To use GPIOP4, SCU88[20] must be 0
# To use GPIOP4, SCU90[28] must be 0
devmem_clear_bit $(scu_addr 88) 20
devmem_clear_bit $(scu_addr 90) 28

gpio_export DEBUG_GPIO_BMC_2 GPIOP4
gpio_set DEBUG_GPIO_BMC_2 1
# set GPIOP4 WDT reset tolerance
gpio_tolerance_fun DEBUG_GPIO_BMC_2 GPIOP4

# DEBUG_GPIO_BMC_3: P5 (125)
# To use GPIOP4, SCU88[20] must be 0
# To use GPIOP5, SCU88[21] must be 0
devmem_clear_bit $(scu_addr 88) 21
devmem_clear_bit $(scu_addr 90) 28

gpio_export DEBUG_GPIO_BMC_3 GPIOP5

# DEBUG_GPIO_BMC_4: P6 (126)
# To use GPIOP4, SCU88[20] must be 0
# To use GPIOP6, SCU88[22] must be 0
devmem_clear_bit $(scu_addr 88) 22
devmem_clear_bit $(scu_addr 90) 28

gpio_export DEBUG_GPIO_BMC_4 GPIOP6

# DEBUG_GPIO_BMC_5: P7 (127)
# To use GPIOP4, SCU88[20] must be 0
# To use GPIOP7, SCU88[23] must be 0
devmem_clear_bit $(scu_addr 88) 23
devmem_clear_bit $(scu_addr 90) 28

gpio_export DEBUG_GPIO_BMC_5 GPIOP7

# DB_PRSNT_BMC_N: Q6 (134)
# To use GPIOQ6, SCU2C[1] must be 0
devmem_clear_bit $(scu_addr 2C) 1

gpio_export DB_PRSNT_BMC_N GPIOQ6

# EXP_UART_EN: S0 (144)
# To use GPIOS0, SCU8C[0] must be 0
devmem_clear_bit $(scu_addr 8C) 0

gpio_export EXP_UART_EN GPIOS0
gpio_set EXP_UART_EN 0

# UART_SEL_IN: S1 (145)
# To use GPIOS1, SCU8C[1] must be 0
devmem_clear_bit $(scu_addr 8C) 1

gpio_export UART_SEL_IN GPIOS1

# BMC_GPIOS4: S4 (148)
# To use GPIOS4, SCU8C[4] must be 0
devmem_clear_bit $(scu_addr 8C) 4

gpio_export BMC_GPIOS4 GPIOS4

# BMC_GPIOS5: S5 (149)
# To use GPIOS5, SCU8C[5] must be 0
devmem_clear_bit $(scu_addr 8C) 5

gpio_export BMC_GPIOS5 GPIOS5

# BMC_GPIOS6: S6 (150)
# To use GPIOS6, SCU8C[6] must be 0
devmem_clear_bit $(scu_addr 8C) 6

gpio_export BMC_GPIOS6 GPIOS6

# BMC_GPIOS7: S7 (151)
# To use GPIOS7, SCU8C[7] must be 0
devmem_clear_bit $(scu_addr 8C) 7

gpio_export BMC_GPIOS7 GPIOS7

# Disable Z0~AB1 Function 3-4
# To use NOR Function, SCU90[31] must be 0
devmem_clear_bit $(scu_addr 90) 31

# DEBUG_RST_BTN_N: Z0 (200)
# To use GPIOZ0, SCUA4[16], and SCU70[19] must be 0
devmem_clear_bit $(scu_addr A4) 16
devmem_clear_bit $(scu_addr 70) 19

gpio_export DEBUG_RST_BTN_N GPIOZ0

#Z1 = 1, Z2 = 0 to make IOM LED defualt OFF
# BMC_ML_PWR_YELLOW_LED: Z1 (201)
# To use GPIOZ1, SCUA4[17], and SCU70[19] must be 0
devmem_clear_bit $(scu_addr A4) 17
devmem_clear_bit $(scu_addr 70) 19

gpio_export BMC_ML_PWR_YELLOW_LED GPIOZ1
gpio_set BMC_ML_PWR_YELLOW_LED 1

# BMC_ML_PWR_BLUE_LED: Z2 (202)
# To use GPIOZ2, SCUA4[18], and SCU70[19] must be 0
devmem_clear_bit $(scu_addr A4) 18
devmem_clear_bit $(scu_addr 70) 19

gpio_export BMC_ML_PWR_BLUE_LED GPIOZ2
gpio_set BMC_ML_PWR_BLUE_LED 0

# PCIE_COMP_TO_IOM_RST_4: Z3 (203)
# To use GPIOZ3, SCUA4[19] must be 0
devmem_clear_bit $(scu_addr A4) 19

gpio_export PCIE_COMP_TO_IOM_RST_4 GPIOZ3

# BMC_GPIOZ4: Z4 (204)
# To use GPIOZ4, SCUA4[20] must be 0
devmem_clear_bit $(scu_addr A4) 20

gpio_export BMC_GPIOZ4 GPIOZ4

# BMC_GPIOZ5: Z5 (205)
# To use GPIOZ5, SCUA4[21] must be 0
devmem_clear_bit $(scu_addr A4) 21

gpio_export BMC_GPIOZ5 GPIOZ5

# BMC_GPIOZ6: Z6 (206)
# To use GPIOZ6, SCUA4[22] must be 0
devmem_clear_bit $(scu_addr A4) 22

gpio_export BMC_GPIOZ6 GPIOZ6

# BMC_GPIOZ7: Z7 (207)
# To use GPIOZ7, SCUA4[23] must be 0
devmem_clear_bit $(scu_addr A4) 23

gpio_export BMC_GPIOZ7 GPIOZ7

# COMP_RST_BTN_N: AA0 (208)
# To use GPIOAA0, SCUA4[24] must be 0
devmem_clear_bit $(scu_addr A4) 24

gpio_export COMP_RST_BTN_N GPIOAA0
gpio_set COMP_RST_BTN_N 1

# CONN_A_PRSNTA: AA1 (209)
# To use GPIOAA1, SCUA4[25] must be 0
devmem_clear_bit $(scu_addr A4) 25

gpio_export CONN_A_PRSNTA GPIOAA1

# CONN_B_PRSNTB_N: AA2 (210)
# To use GPIOAA2, SCUA4[26] must be 0
devmem_clear_bit $(scu_addr A4) 26

gpio_export CONN_B_PRSNTB_N GPIOAA2

# FM_TPM_PRSNT_N: AA3 (211)
# To use GPIOAA3, SCUA4[27] must be 0
devmem_clear_bit $(scu_addr A4) 27

gpio_export FM_TPM_PRSNT_N GPIOAA3

# BMC_SELF_HW_RST: AA4 (212) For TPM; don't need to implement

# IOM_FULL_PWR_EN: AA7 (215), PS
# To use GPIOAA7, SCUA4[31] must be 0
devmem_clear_bit $(scu_addr A4) 31

gpio_export IOM_FULL_PWR_EN GPIOAA7
# set GPIOAA7 WDT reset tolerance
gpio_tolerance_fun IOM_FULL_PWR_EN GPIOAA7

# IOM_LOC_INS_N: AB0 (216)
# To use GPIOAB0, SCUA8[0] must be 0
devmem_clear_bit $(scu_addr A8) 0

gpio_export IOM_LOC_INS_N GPIOAB0

# IOM_FULL_PGOOD: AB1 (217)
# To use GPIOAB1, SCUA8[1] must be 0
devmem_clear_bit $(scu_addr A8) 1

gpio_export IOM_FULL_PGOOD GPIOAB1

# BMC_SELF_HW_RST: AB2 (218)
# Set GPIOAB2 to funtion2:WDTRST1 , SCUA8[2] must be 1, and SCU94[1:0] must be 0
devmem_set_bit $(scu_addr A8) 2
devmem_clear_bit $(scu_addr 94) 0
devmem_clear_bit $(scu_addr 94) 1

# === PCA9555 GPIOs ===
# ML_INS_N: P00 (472)
pca9555_gpio_export ML_INS_N 0
# FAN_0_INS_N: P01 (473)
pca9555_gpio_export FAN_0_INS_N 1
# FAN_1_INS_N: P02 (474)
pca9555_gpio_export FAN_1_INS_N 2
# FAN_2_INS_N: P03 (475)
pca9555_gpio_export FAN_2_INS_N 3
# FAN_3_INS_N: P04 (476)
pca9555_gpio_export FAN_3_INS_N 4
# ANOTHER_IOM_INS_N: P05 (477)
pca9555_gpio_export ANOTHER_IOM_INS_N 5
# SCC_A_INS: P06 (478)
pca9555_gpio_export SCC_A_INS 6
# SCC_B_INS: P07 (479)
pca9555_gpio_export SCC_B_INS 7

# SCC_TYPE_0: P10 (480)
pca9555_gpio_export SCC_TYPE_0 8
# SCC_TYPE_1: P11 (481)
pca9555_gpio_export SCC_TYPE_1 9
# SCC_TYPE_2: P12 (482)
pca9555_gpio_export SCC_TYPE_2 10
# SCC_TYPE_3: P13 (483)
pca9555_gpio_export SCC_TYPE_3 11
# SCC_STBY_PGOOD: P14 (484)
pca9555_gpio_export SCC_STBY_PGOOD 12
# SCC_FULL_PGOOD: P15 (485)
pca9555_gpio_export SCC_FULL_PGOOD 13
# ML_PGOOD: P16 (486)
pca9555_gpio_export ML_PGOOD 14
# DRAWER_CLOSED_N: P17 (487)
pca9555_gpio_export DRAWER_CLOSED_N 15
