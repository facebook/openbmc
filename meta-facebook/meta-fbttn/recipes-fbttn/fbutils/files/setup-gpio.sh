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
#This PIN  GPIOA0 (MAC0 ISR ) will cause kernel error message
# USB_OCS_N1: A0 (0)
# To use GPIOA0, SCU80[0]
#devmem_clear_bit $(scu_addr 80) 0

#gpio_export A0

# This is a workaround for MAC1LINK
devmem 0x1e660050 w 0xA954F

# BMC_TO_EXP_RESET: A2 (2)
# To use GPIOA2, SCU80[2], and SCU80[15] must be 0
devmem_clear_bit $(scu_addr 80) 2
devmem_clear_bit $(scu_addr 80) 15

gpio_tolerance_fun A2

# SYS_PWR_LED: A3 (3)
# To use GPIOA3, SCU80[3] must be 0
devmem_clear_bit $(scu_addr 80) 3

gpio_set A3 1

# PWRBTN_OUT_N: D2 (26)
# To use GPIOD2, SCU90[1], SCU8C[9], and SCU70[21] must be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8C) 9
devmem_clear_bit $(scu_addr 70) 21

gpio_export D2

# COMP_PWR_BTN_N: D3 (27)
# To use GPIOD3, SCU90[1], SCU8C[9], and SCU70[21] must be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8C) 9
devmem_clear_bit $(scu_addr 70) 21

gpio_set D3 1
# set GPIOD3 WDT reset tolerance
gpio_tolerance_fun D3

# RSTBTN_OUT_N: D4 (28)
# To use GPIOD4, SCU90[1], SCU8C[10], and SCU70[21] must be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8C) 10
devmem_clear_bit $(scu_addr 70) 21

gpio_export D4

# SYS_RESET_N_OUT: D5 (29)
# To use GPIOD5, SCU90[1], SCU8C[10], and SCU70[21] must be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8C) 10
devmem_clear_bit $(scu_addr 70) 21

gpio_set D5 1
# set GPIOD5 WDT reset tolerance
gpio_tolerance_fun D5

# M2_1_PRESENT_N: E0 (32)
# To use GPIOE0, SCU80[16], SCU8C[12], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 16
devmem_clear_bit $(scu_addr 8C) 12
devmem_clear_bit $(scu_addr 70) 22

gpio_export E0

# M2_2_PRESENT_N: E1 (33)
# To use GPIOE1, SCU80[17], SCU8C[12], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 17
devmem_clear_bit $(scu_addr 8C) 12
devmem_clear_bit $(scu_addr 70) 22

gpio_export E1

# M2_1_FAULT_N: E2 (34)
# To use GPIOE2, SCU80[18], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 18
devmem_clear_bit $(scu_addr 8C) 13
devmem_clear_bit $(scu_addr 70) 22

gpio_export E2

# M2_2_FAULT_N: E3 (35)
# To use GPIOE3, SCU80[19], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 19
devmem_clear_bit $(scu_addr 8C) 13
devmem_clear_bit $(scu_addr 70) 22

gpio_export E3

# BMC_EXT1_LED_B: E4 (36)
# To use GPIOE4, SCU80[20], SCU8C[14], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 20
devmem_clear_bit $(scu_addr 8C) 14
devmem_clear_bit $(scu_addr 70) 22

gpio_set E4 0

# BMC_EXT1_LED_Y: E5 (37)
# To use GPIOE5, SCU80[21], SCU8C[14], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 21
devmem_clear_bit $(scu_addr 8C) 14
devmem_clear_bit $(scu_addr 70) 22

gpio_set E5 1

# BMC_EXT2_LED_B: E6 (38)
# To use GPIOE6, SCU80[22], SCU8C[15], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 22
devmem_clear_bit $(scu_addr 8C) 15
devmem_clear_bit $(scu_addr 70) 22

gpio_set E6 0

# BMC_EXT2_LED_Y: E7 (39)
# To use GPIOE7, SCU80[23], SCU8C[15], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 23
devmem_clear_bit $(scu_addr 8C) 15
devmem_clear_bit $(scu_addr 70) 22

gpio_set E7 1

# Because SCC power sequence is controlled by HW on EVT.
# So we just configure SCC PWR pins to GPIO and does not export their GPIO node.
# SCC PWR pins: GPIOF0, GPIOF1, and GPIOF4
# SCC_LOC_FULL_PWR_EN: F0 (40), PS
# To use GPIOF0, LHCR[0], SCU90[30], and SCU80[24] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 24

# SCC_RMT_FULL_PWR_EN: F1 (41)
# To use GPIOF0, LHCR[0], SCU90[30], and SCU80[25] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 25

# EXP_SPARE_0: F2 (42), EXP_SPARE_1: F3 (43)
# To use GPIOF2, LHCR[0], SCU90[30], and SCU80[26] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 26
# To use GPIOF3, LHCR[0], SCU90[30], and SCU80[27] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 27

gpio_export F2
gpio_export F3


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

gpio_export F5

# P12V_A_PGOOD: F6 (46)
# To use GPIOF6, LHCR[0], and SCU80[30] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 80) 30

gpio_export F6

# SCC_RMT_TYPE_0: F7 (47)
# To use GPIOF7, LHCR[0], SCU90[30], and SCU80[31] must be 0
devmem_clear_bit $(lpc_addr A0) 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 31

gpio_export F7

# SLOTID_0: G0 (48), SLOTID_1: G1 (49)
# To use GPIOG0, SCU84[0] must be 0
devmem_clear_bit $(scu_addr 84) 0
# To use GPIOG1, SCU84[1] must be 0
devmem_clear_bit $(scu_addr 84) 1

gpio_export G0
gpio_export G1

# COMP_SPARE_0: G2 (50), COMP_SPARE_1: G3 (51))
# To use GPIOG2, SCU84[2] must be 0
devmem_clear_bit $(scu_addr 84) 2
# To use GPIOG3, SCU84[3] must be 0
devmem_clear_bit $(scu_addr 84) 3

gpio_export G2
gpio_export G3


# I2C_COMP_ALERT_N: G7 (55)
# To use GPIOG7, SCU94[12], and SCU84[7] must be 0
devmem_clear_bit $(scu_addr 94) 12
devmem_clear_bit $(scu_addr 84) 7

gpio_export G7

# LED_POSTCODE_0: H0 (56)
# To use GPIOH0, SCU94[5], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 5
devmem_clear_bit $(scu_addr 90) 7

gpio_set H0 0

# LED_POSTCODE_1: H1 (57)
# To use GPIOH1, SCU94[5], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 5
devmem_clear_bit $(scu_addr 90) 7

gpio_set H1 0

# LED_POSTCODE_2: H2 (58)
# To use GPIOH2, SCU94[6], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 6
devmem_clear_bit $(scu_addr 90) 7

gpio_set H2 0

# LED_POSTCODE_3: H3 (59)
# To use GPIOH3, SCU94[6], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 6
devmem_clear_bit $(scu_addr 90) 7

gpio_set H3 0

# LED_POSTCODE_4: H4 (60)
# To use GPIOH4, SCU94[7], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 7
devmem_clear_bit $(scu_addr 90) 7

gpio_set H4 0

# LED_POSTCODE_5: H5 (61)
# To use GPIOH5, SCU94[7], and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 94) 7
devmem_clear_bit $(scu_addr 90) 7

gpio_set H5 0

# LED_POSTCODE_6: H6 (62)
# To use GPIOH6, SCU90[7] must be 0
devmem_clear_bit $(scu_addr 90) 7

gpio_set H6 0

# LED_POSTCODE_7: H7 (63)
# To use GPIOH7, SCU90[7] must be 0
devmem_clear_bit $(scu_addr 90) 7

gpio_set H7 0

# BOARD_REV_0: J0 (72), BOARD_REV_1: J1 (73), BOARD_REV_2: J2 (74)
# To use GPIOJ0, SCU84[8] must be 0
devmem_clear_bit $(scu_addr 84) 8
# To use GPIOJ1, SCU84[9] must be 0
devmem_clear_bit $(scu_addr 84) 9
# To use GPIOJ2, SCU84[10] must be 0
devmem_clear_bit $(scu_addr 84) 10

gpio_export J0
gpio_export J1
gpio_export J2


# PCIE_WAKE_N: J3 (75)
# To use GPIOJ3, SCU84[11] must be 0
devmem_clear_bit $(scu_addr 84) 11

gpio_export J3

# IOM_TYPE0: J4 (76)
# To use GPIOJ4, SCU84[12] must be 0
# To use GPIOJ4, SCU94[8] must be 0
devmem_clear_bit $(scu_addr 84) 12
devmem_clear_bit $(scu_addr 94) 8

gpio_export J4

# IOM_TYPE1: J5 (77)
# To use GPIOJ5, SCU84[13] must be 0
# To use GPIOJ5, SCU94[8] must be 0
devmem_clear_bit $(scu_addr 84) 13
devmem_clear_bit $(scu_addr 94) 8

gpio_export J5

# IOM_TYPE2: J6 (78)
# To use GPIOJ6, SCU84[14] must be 0
# To use GPIOJ6, SCU94[9] must be 0
devmem_clear_bit $(scu_addr 84) 14
devmem_clear_bit $(scu_addr 94) 9

gpio_export J6

# IOM_TYPE3: J7 (79)
# To use GPIOJ7, SCU84[15] must be 0
# To use GPIOJ7, SCU94[9] must be 0
devmem_clear_bit $(scu_addr 84) 15
devmem_clear_bit $(scu_addr 94) 9

gpio_export J7

# BMC_RMT_HEARTBEAT(TACH): O0 (112)
# To use GPIOO0, SCU88[8] must be 0
devmem_clear_bit $(scu_addr 88) 8

gpio_export O0

# BMC_LOC_HEARTBEAT: O1 (113)
# To use GPIOO1, SCU88[9] must be 0
devmem_clear_bit $(scu_addr 88) 9

gpio_set O1 1

# FLA0_WP_N: O2 (114)
# To use GPIOO1, SCU88[10] must be 0
devmem_clear_bit $(scu_addr 88) 10

gpio_export O2

# ENCL_FAULT_LED: O3 (115)
# To use GPIOO1, SCU88[11] must be 0
devmem_clear_bit $(scu_addr 88) 11

gpio_set O3 0

# SCC_LOC_HEARTBEAT: O4 (116)
# To use GPIOO4, SCU88[12] must be 0
devmem_clear_bit $(scu_addr 88) 12

gpio_export O4

# SCC_RMT_HEARTBEAT: O5 (117)
# To use GPIOO5, SCU88[13] must be 0
devmem_clear_bit $(scu_addr 88) 13

gpio_export O5

# COMP_POWER_FAIL_N: O6 (118)
# To use GPIOO6, SCU88[14] must be 0
devmem_clear_bit $(scu_addr 88) 14

gpio_export O6

# COMP_PWR_EN: O7 (119), PS
# To use GPIOO7, SCU88[15] must be 0
devmem_clear_bit $(scu_addr 88) 15

# set GPIOO7 WDT reset tolerance
gpio_tolerance_fun O7

# Disable GPIOP internal pull down
devmem_set_bit $(scu_addr 8C) 31

# DEBUG_GPIO_BMC_6: P0 (120)
# To use GPIOP0, SCU88[16] must be 0
devmem_clear_bit $(scu_addr 88) 16

gpio_set P0 1
# set GPIOP0 WDT reset tolerance
gpio_tolerance_fun P0

# COMP_FAST_THROTTLE_N: P2 (122)
# To use GPIOP2, SCU88[18] must be 0
devmem_clear_bit $(scu_addr 88) 18

# TODO: Disable the CPU Throttle PIN to fix the leakage issue from BMC to ML
gpio_export P2

# DEBUG_PWR_BTN_N: P3 (123)
# To use GPIOP3, SCU88[19] must be 0
devmem_clear_bit $(scu_addr 88) 19

gpio_export P3

# DEBUG_GPIO_BMC_2: P4 (124)
# To use GPIOP4, SCU88[20] must be 0
# To use GPIOP4, SCU90[28] must be 0
devmem_clear_bit $(scu_addr 88) 20
devmem_clear_bit $(scu_addr 90) 28

gpio_set P4 1
# set GPIOP4 WDT reset tolerance
gpio_tolerance_fun P4

# DEBUG_GPIO_BMC_3: P5 (125)
# To use GPIOP4, SCU88[20] must be 0
# To use GPIOP5, SCU88[21] must be 0
devmem_clear_bit $(scu_addr 88) 21
devmem_clear_bit $(scu_addr 90) 28

gpio_export P5

# DEBUG_GPIO_BMC_4: P6 (126)
# To use GPIOP4, SCU88[20] must be 0
# To use GPIOP6, SCU88[22] must be 0
devmem_clear_bit $(scu_addr 88) 22
devmem_clear_bit $(scu_addr 90) 28

gpio_export P6

# DEBUG_GPIO_BMC_5: P7 (127)
# To use GPIOP4, SCU88[20] must be 0
# To use GPIOP7, SCU88[23] must be 0
devmem_clear_bit $(scu_addr 88) 23
devmem_clear_bit $(scu_addr 90) 28

gpio_export P7

# DB_PRSNT_BMC_N: Q6 (134)
# To use GPIOQ6, SCU2C[1] must be 0
devmem_clear_bit $(scu_addr 2C) 1

gpio_export Q6

# EXP_UART_EN: S0 (144)
# To use GPIOS0, SCU8C[0] must be 0
devmem_clear_bit $(scu_addr 8C) 0

gpio_set S0 0

# UART_SEL_IN: S1 (145)
# To use GPIOS1, SCU8C[1] must be 0
devmem_clear_bit $(scu_addr 8C) 1

gpio_export S1

# BMC_GPIOS4: S4 (148)
# To use GPIOS4, SCU8C[4] must be 0
devmem_clear_bit $(scu_addr 8C) 4

gpio_export S4

# BMC_GPIOS5: S5 (149)
# To use GPIOS5, SCU8C[5] must be 0
devmem_clear_bit $(scu_addr 8C) 5

gpio_export S5

# BMC_GPIOS6: S6 (150)
# To use GPIOS6, SCU8C[6] must be 0
devmem_clear_bit $(scu_addr 8C) 6

gpio_export S6

# BMC_GPIOS7: S7 (151)
# To use GPIOS7, SCU8C[7] must be 0
devmem_clear_bit $(scu_addr 8C) 7

gpio_export S7

# Disable Z0~AB1 Function 3-4
# To use NOR Function, SCU90[31] must be 0
devmem_clear_bit $(scu_addr 90) 31

# DEBUG_RST_BTN_N: Z0 (200)
# To use GPIOZ0, SCUA4[16], and SCU70[19] must be 0
devmem_clear_bit $(scu_addr A4) 16
devmem_clear_bit $(scu_addr 70) 19

gpio_export Z0

#Z1 = 1, Z2 = 0 to make IOM LED defualt OFF
# BMC_ML_PWR_YELLOW_LED: Z1 (201)
# To use GPIOZ1, SCUA4[17], and SCU70[19] must be 0
devmem_clear_bit $(scu_addr A4) 17
devmem_clear_bit $(scu_addr 70) 19

gpio_set Z1 1

# BMC_ML_PWR_BLUE_LED: Z2 (202)
# To use GPIOZ2, SCUA4[18], and SCU70[19] must be 0
devmem_clear_bit $(scu_addr A4) 18
devmem_clear_bit $(scu_addr 70) 19

gpio_set Z2 0

# BMC_GPIOZ3: Z3 (203)
# To use GPIOZ3, SCUA4[19] must be 0
devmem_clear_bit $(scu_addr A4) 19

gpio_export Z3

# BMC_GPIOZ4: Z4 (204)
# To use GPIOZ4, SCUA4[20] must be 0
devmem_clear_bit $(scu_addr A4) 20

gpio_export Z4

# BMC_GPIOZ5: Z5 (205)
# To use GPIOZ5, SCUA4[21] must be 0
devmem_clear_bit $(scu_addr A4) 21

gpio_export Z5

# BMC_GPIOZ6: Z6 (206)
# To use GPIOZ6, SCUA4[22] must be 0
devmem_clear_bit $(scu_addr A4) 22

gpio_export Z6

# BMC_GPIOZ7: Z7 (207)
# To use GPIOZ7, SCUA4[23] must be 0
devmem_clear_bit $(scu_addr A4) 23

gpio_export Z7

# COMP_RST_BTN_N: AA0 (208)
# To use GPIOAA0, SCUA4[24] must be 0
devmem_clear_bit $(scu_addr A4) 24

gpio_set AA0 1

# CONN_A_PRSNTA: AA1 (209)
# To use GPIOAA1, SCUA4[25] must be 0
devmem_clear_bit $(scu_addr A4) 25

gpio_export AA1

# CONN_B_PRSNTB_N: AA2 (210)
# To use GPIOAA2, SCUA4[26] must be 0
devmem_clear_bit $(scu_addr A4) 26

gpio_export AA2

# FM_TPM_PRSNT_N: AA3 (211)
# To use GPIOAA3, SCUA4[27] must be 0
devmem_clear_bit $(scu_addr A4) 27

gpio_export AA3

# BMC_SELF_HW_RST: AA4 (212) For TPM; don't need to implement
# To use GPIOAA4, SCUA4[28] must be 0
#devmem_clear_bit $(scu_addr A4) 28

#gpio_export_out AA4

# COMP_SPARE_1: AA5 (213)
# To use GPIOAA5, SCUA4[29] must be 0
devmem_clear_bit $(scu_addr A4) 29

gpio_set AA5 0

# IOM_FULL_PWR_EN: AA7 (215), PS
# To use GPIOAA7, SCUA4[31] must be 0
devmem_clear_bit $(scu_addr A4) 31

# set GPIOAA7 WDT reset tolerance
gpio_tolerance_fun AA7

# IOM_LOC_INS_N: AB0 (216)
# To use GPIOAB0, SCUA8[0] must be 0
devmem_clear_bit $(scu_addr A8) 0

gpio_export AB0

# IOM_FULL_PGOOD: AB1 (217)
# To use GPIOAB1, SCUA8[1] must be 0
devmem_clear_bit $(scu_addr A8) 1

gpio_export AB1

# BMC_SELF_HW_RST: AB2 (218)
# Set GPIOAB2 to funtion2:WDTRST1 , SCUA8[2] must be 1, and SCU94[1:0] must be 0
devmem_set_bit $(scu_addr A8) 2
devmem_clear_bit $(scu_addr 94) 0
devmem_clear_bit $(scu_addr 94) 1

# PCA9555 GPIO export; GPIO number 472~487
pca9555_pin="472"
while [ "$pca9555_pin" -lt "488" ]
do
	#echo "pca9555_pin: $pca9555_pin"
	pca9555_gpio_export $pca9555_pin
	pca9555_pin=$(($pca9555_pin+1))
done
