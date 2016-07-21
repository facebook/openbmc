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

# FCB_HW_REVISION: GPIOY1
# DPB_HW_REVISION: GPIOY2
# To use GPIOY1, SCUA4[9] and STRAP[19] must be 0
# To use GPIOY2, SCUA4[10] and STRAP[19] must be 0
devmem_set_bit $(scu_addr 70) 19
devmem_clear_bit $(scu_addr a4) 9
devmem_clear_bit $(scu_addr a4) 10

gpio_get Y1
gpio_get Y2


# PE1_PERST#: GPIOB0
# To use GPIOB0, SCU80[8] must be 0
devmem_clear_bit $(scu_addr 80) 8

gpio_set B0 1

# BMC_ADD1_R: GPIOE4 
# To use GPIOE4 (36), SCU80[20], SCU8C[14], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 20
devmem_clear_bit $(scu_addr 8C) 14
devmem_clear_bit $(scu_addr 70) 22

gpio_get E4

# BMC_ADD2_R: GPIOE5
# To use GPIOE5 (37), SCU80[21], SCU8C[14], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 21
devmem_clear_bit $(scu_addr 8C) 14
devmem_clear_bit $(scu_addr 70) 22

gpio_get E5

# System SPI
# Strap 12 must be 0 and Strape 13 must be 1
devmem_clear_bit $(scu_addr 70) 12
devmem_set_bit $(scu_addr 70) 13

# LED POST CODES: 8 GPIO signals

# LED_POSTCODE_0: GPIOG0 (48)
# To use GPIOG0, SCU84[0] must be 0
devmem_clear_bit $(scu_addr 84) 0

gpio_set G0 0

# LED_POSTCODE_1: GPIOG1 (49)
# To use GPIOG1, SCU84[1] must be 0
devmem_clear_bit $(scu_addr 84) 1

gpio_set G1 0

# LED_POSTCODE_2: GPIOG2 (50)
# To use GPIOG2, SCU84[2] must be 0
devmem_clear_bit $(scu_addr 84) 2

gpio_set G2 0

# LED_POSTCODE_3: GPIOG3 (51)
# To use GPIOG3, SCU84[3] must be 0
devmem_clear_bit $(scu_addr 84) 3

gpio_set G3 0

# LED_POSTCODE_4: GPIOJ0 (72)
# To use GPIOJ0, SCU84[8] must be 0
devmem_clear_bit $(scu_addr 84) 8

gpio_set J0 0

# LED_POSTCODE_5: GPIOJ1 (73)
# To use GPIOJ1, SCU84[9] must be 0
devmem_clear_bit $(scu_addr 84) 9

gpio_set J1 0

# LED_POSTCODE_6: GPIOJ2 (74)
# To use GPIOJ2, SCU84[10] must be 0
devmem_clear_bit $(scu_addr 84) 10

gpio_set J2 0

# LED_POSTCODE_7: GPIOJ3 (75)
# To use GPIOJ3, SCU84[11] must be 0
devmem_clear_bit $(scu_addr 84) 11

gpio_set J3 0

# BMC_ID_LED: GPIOH1 (57)
# To use GPIOH1, SCU90[6], SCU90[7] must be 0
devmem_clear_bit $(scu_addr 90) 6
devmem_clear_bit $(scu_addr 90) 7

gpio_set H1 1

# HB_OUT_BMC: GPIOO3 (115)
# To use GPIOO3, SCU88[11] must be 0
devmem_clear_bit $(scu_addr 88) 11

gpio_set O3 1

# BMC_ENCLOSURE_LED : GPIOP7 (127)
# To use GPIOP7, SCU88[23] must be 0
devmem_clear_bit $(scu_addr 88) 23

gpio_set P7 1

# UART_COUNT : GPIOP5 (125)
gpio_get P5

# DP_RST_N : GPIOG6 (54)
# To use GPIOG6, SCU84[6] must be 0
devmem_clear_bit $(scu_addr 84) 6

gpio_get G6

# BMC_UART_SWITCH : GPIOP3 (123)
# To use GPIOP3, SCU88[19] must be 0 
devmem_clear_bit $(scu_addr 88) 19

gpio_set P3 0;

# BMC_SSD_RST#0 - GPIOM1 (97) 
# To use GPIOM1 (97), SCU90[4], SCU90[5], and SCU84[25] must be 0
devmem_clear_bit $(scu_addr 90) 4
devmem_clear_bit $(scu_addr 90) 5
devmem_clear_bit $(scu_addr 84) 25

gpio_set M1 1

# BMC_SSD_RST#1 - GPIOM2 (98) 
# To use GPIOM2 (98), SCU90[4], SCU90[5], and SCU84[26] must be 0
devmem_clear_bit $(scu_addr 84) 26

gpio_set M2 1

# BMC_SSD_RST#2 - GPIOM3 (99) 
# To use GPIOM3 (99), SCU90[4], SCU90[5], and SCU84[27] must be 0
devmem_clear_bit $(scu_addr 84) 27

gpio_set M3 1

# BMC_SSD_RST#3 - GPIOM4 (100) 
# To use GPIOM4 (100), SCU90[4], SCU90[5], and SCU84[28] must be 0
devmem_clear_bit $(scu_addr 84) 28

gpio_set M4 1

# BMC_SSD_RST#4 - GPIOM5 (101) 
# To use GPIOM5 (101), SCU90[4], SCU90[5], and SCU84[29] must be 0
devmem_clear_bit $(scu_addr 84) 29

gpio_set M5 1

# BMC_SSD_RST#5 - GPIOM6 (102) 
# To use GPIOM6 (102), SCU90[4], SCU90[5], and SCU84[30] must be 0
devmem_clear_bit $(scu_addr 84) 30

gpio_set M6 1

# BMC_SSD_RST#6 - GPIOM7 (103)
# To use GPIOM7 (103), SCU90[4], SCU90[5], and SCU84[31] must be 0
devmem_clear_bit $(scu_addr 84) 31

gpio_set M7 1

# BMC_SSD_RST#7 - GPIOF0 (40)
# To use GPIOF0 (40), SCU80[24] must be 0
devmem_clear_bit $(scu_addr 80) 24

gpio_set F0 1

# BMC_SSD_RST#8 - GPIOF1 (41)
# To use GPIOF1 (41), SCU80[25] and SCUA4[12] must be 0
devmem_clear_bit $(scu_addr 80) 25
devmem_clear_bit $(scu_addr A4) 12 

gpio_set F1 1

# BMC_SSD_RST#9 - GPIOF2 (42)
# To use GPIOF2 (42), SCU80[26] and SCUA4[13] must be 0
devmem_clear_bit $(scu_addr 80) 26
devmem_clear_bit $(scu_addr A4) 13 

gpio_set F2 1

# BMC_SSD_RST#10 - GPIOF3 (43)
# To use GPIOF3 (43), SCU80[27] and SCUA4[14] must be 0
devmem_clear_bit $(scu_addr 80) 27
devmem_clear_bit $(scu_addr A4) 14 

gpio_set F3 1

# BMC_SSD_RST#11 - GPIOF4 (44)
# To use GPIOF4 (44), SCU80[28] must be 0
devmem_clear_bit $(scu_addr 80) 28

gpio_set F4 1

# BMC_SSD_RST#12 - GPIOF5 (45)
# To use GPIOF5 (45), SCU80[29] and SCUA4[15] must be 0
devmem_clear_bit $(scu_addr 80) 29
devmem_clear_bit $(scu_addr A4) 15 

gpio_set F5 1

# BMC_SSD_RST#13 - GPIOF6 (46)
# To use GPIOF6 (46), SCU80[30] must be 0
devmem_clear_bit $(scu_addr 80) 30

gpio_set F6 1

# BMC_SSD_RST#14 - GPIOF7 (47)
# To use GPIOF7 (47), SCU80[31] must be 0
devmem_clear_bit $(scu_addr 80) 31

gpio_set F7 1

# LED_DR_LED1 - GPION2 (106)
# To use GPION2 (106), SCU88[2] must be 0
devmem_clear_bit $(scu_addr 88) 2

gpio_set N2 1

# PMC_PLX_BOARD - GPIOD6
# To use GPIOD6, SCU90[1], SCU8C[11], STRAP[21] must be 0
devmem_clear_bit $(scu_addr 70) 21
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8c) 11

gpio_get D6

# FLA_CS1_R - ROMCS1#
# To use ROMCS1#, SCU88[24] must be 1
devmem_set_bit $(scu_addr 88) 24

# I2C_MUX_RESET_PEB - GPIOQ6
gpio_set Q6 1

# TCA9554_INT_N - GPIOQ7
gpio_get Q7

# BMC_SELF_TRAY - GPION4
# To use GPION4, SCU90[5:4] and SCU88[4] must be 0
devmem_clear_bit $(scu_addr 90) 5
devmem_clear_bit $(scu_addr 90) 4
devmem_clear_bit $(scu_addr 88) 4

gpio_get N4

# BMC_FW_DET_BOARD_VER_0 - GPION1
# BMC_FW_DET_BOARD_VER_1 - GPION6
# BMC_FW_DET_BOARD_VER_2 - GPION7
# To use GPION1, SCU90[5:4] and SCU88[1] must be 0
# To use GPION6, SCU90[5:4] and SCU88[6] must be 0
# To use GPION7, SCU90[5:4] and SCU88[7] must be 0
devmem_clear_bit $(scu_addr 90) 5
devmem_clear_bit $(scu_addr 90) 4
devmem_clear_bit $(scu_addr 88) 1
devmem_clear_bit $(scu_addr 88) 6
devmem_clear_bit $(scu_addr 88) 7

gpio_get N1
gpio_get N6
gpio_get N7

# RST_BMC_EXTRST_N - EXTRST#
# To use EXTRST#, SCU80[15] must be 1, SCU90[31] must be 0, SCU3C[3] must be 1
devmem_set_bit $(scu_addr 80) 15
devmem_clear_bit $(scu_addr 90) 31
devmem_set_bit $(scu_addr 3c) 3

# BMC_SELF_HW_RST - GPIOG5
# To use GPIOG5, Strap[23] and SCU84[5] must be 0
devmem_clear_bit $(scu_addr 70) 23
devmem_clear_bit $(scu_addr 84) 5

gpio_set G5 0

# To use ROMD4, SCU8C[0] must be 1, SCU94[1:0] must be 0
# To use ROMD5, SCU8C[1] must be 1, SCU94[1:0] must be 0
# To use ROMD6, SCU8C[2] must be 1, SCU94[1:0] must be 0
# To use ROMD7, SCU8C[3] must be 1, SCU94[1:0] must be 0
devmem_clear_bit $(scu_addr 94) 1
devmem_clear_bit $(scu_addr 94) 0
devmem_set_bit $(scu_addr 8c) 0
devmem_set_bit $(scu_addr 8c) 1
devmem_set_bit $(scu_addr 8c) 2
devmem_set_bit $(scu_addr 8c) 3

# CBL_PRSNT_N_1 - GPIOL0
# CBL_PRSNT_N_2 - GPIOL1
# CBL_PRSNT_N_3 - GPIOL2
# CBL_PRSNT_N_4 - GPIOL3
# CBL_PRSNT_N_5 - GPIOL4
# CBL_PRSNT_N_6 - GPIOL5
# CBL_PRSNT_N_7 - GPIOL6
# CBL_PRSNT_N_8 - GPIOL7
# To use GPIOL0, SCU84[16] must be 0
# To use GPIOL1, SCU90[5:4] and SCU84[17] must be 0
# To use GPIOL2, SCU90[5:4] and SCU84[18] must be 0
# To use GPIOL3, SCU90[5:4] and SCU84[19] must be 0
# To use GPIOL4, SCU90[5:4] and SCU84[20] must be 0
# To use GPIOL5, SCU90[5:4] and SCU84[21] must be 0
# To use GPIOL6, SCU90[5:4] and SCU84[22] must be 0
# To use GPIOL7, SCU90[5:4] and SCU84[23] must be 0
devmem_clear_bit $(scu_addr 90) 5
devmem_clear_bit $(scu_addr 90) 4
devmem_clear_bit $(scu_addr 84) 16
devmem_clear_bit $(scu_addr 84) 17
devmem_clear_bit $(scu_addr 84) 18
devmem_clear_bit $(scu_addr 84) 19
devmem_clear_bit $(scu_addr 84) 20
devmem_clear_bit $(scu_addr 84) 21
devmem_clear_bit $(scu_addr 84) 22
devmem_clear_bit $(scu_addr 84) 23

gpio_get L0
gpio_get L1
gpio_get L2
gpio_get L3
gpio_get L4
gpio_get L5
gpio_get L6
gpio_get L7

exit 0
