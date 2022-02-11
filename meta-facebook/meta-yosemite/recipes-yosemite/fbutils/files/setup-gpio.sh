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

# Set up to read the board revision pins, Y0, Y1, Y2
devmem_set_bit $(scu_addr 70) 19
devmem_clear_bit $(scu_addr a4) 8
devmem_clear_bit $(scu_addr a4) 9
devmem_clear_bit $(scu_addr a4) 10

gpio_export Y0
gpio_export Y1
gpio_export Y2

# SLOT1_PRSNT_N, GPIOH5 (61)
# GPIOH5(61): SCU90[6], SCU90[7] shall be 0
devmem_clear_bit $(scu_addr 90) 6
devmem_clear_bit $(scu_addr 90) 7

gpio_export H5

# SLOT2_PRSNT_N, GPIOH4 (60)
# GPIOH4(60): SCU90[6], SCU90[7] shall be 0
gpio_export H4

# SLOT3_PRSNT_N, GPIOH7 (63)
# GPIOH7(63): SCU90[6], SCU90[7] shall be 0
gpio_export H7

# SLOT4_PRSNT_N, GPIOH6 (62)
# GPIOH6(62): SCU90[6], SCU90[7] shall be 0
gpio_export H6

# BMC_PWR_BTN_IN_N, uServer power button in, on GPIO D0(24)
gpio_export D0

# PWR_SLOT1_BTN_N, 1S Server power out, on GPIO D3
# GPIOD3(27): SCU90[1], SCU8C[9], and SCU70[21] shall be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8c) 9
devmem_clear_bit $(scu_addr 70) 21

gpio_set D3 1

# PWR_SLOT2_BTN_N, 1S Server power out, on GPIO D1
# Make sure the Power Control Pin is Set properly
# GPIOD1(25): SCU90[1], SCU8C[8], and SCU70[21] shall be 0

devmem_clear_bit $(scu_addr 8c) 8

gpio_set D1 1

# PWR_SLOT3_BTN_N, 1S Server power out, on GPIO D7
# GPIOD7(31): SCU90[1], SCU8C[11], and SCU70[21] shall be 0
devmem_clear_bit $(scu_addr 8c) 11

gpio_set D7 1

# PWR_SLOT4_BTN_N, 1S Server power out, on GPIO D5
# GPIOD5(29): SCU90[1], SCU8C[10], and SCU70[21] shall be 0
devmem_clear_bit $(scu_addr 8c) 10

gpio_set D5 1

# SMB_SLOT0_NIC_ALERT_N, alert for 1S Server NIC I2C, GPIO B0
devmem_clear_bit $(scu_addr 80) 8

gpio_export B0

# Enable P3V3: GPIOS1(145)
# To use GPIOS1 (145), SCU8C[1], SCU94[0], and SCU94[1] must be 0
devmem_clear_bit $(scu_addr 8C) 1
devmem_clear_bit $(scu_addr 94) 0
devmem_clear_bit $(scu_addr 94) 1

gpio_set S1 1

# PWRGD_P3V3: GPIOS2(146)
# To use GPIOS2 (146), SCU8C[2], SCU94[0], and SCU94[1] must be 0
devmem_clear_bit $(scu_addr 8C) 2
devmem_clear_bit $(scu_addr 94) 0
devmem_clear_bit $(scu_addr 94) 1

# Setup GPIOs to Mux Enable: GPIOS3(147), Channel Select: GPIOE4(36), GPIOE5(37)

# To use GPIOS3 (147), SCU8C[3], SCU94[0], and SCU94[1] must be 0
devmem_clear_bit $(scu_addr 8C) 3
devmem_clear_bit $(scu_addr 94) 0
devmem_clear_bit $(scu_addr 94) 1

# To use GPIOE4 (36), SCU80[20], SCU8C[14], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 20
devmem_clear_bit $(scu_addr 8C) 14
devmem_clear_bit $(scu_addr 70) 22

# To use GPIOE5 (37), SCU80[21], SCU8C[14], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 21
devmem_clear_bit $(scu_addr 8C) 14
devmem_clear_bit $(scu_addr 70) 22

gpio_set S3 0
gpio_set E4 1
gpio_set E5 0

# BMC_HEARTBEAT_N, heartbeat LED, GPIO Q7(135)
devmem_clear_bit $(scu_addr 90) 28

gpio_set Q7 0

# USB_OC_N, resettable fuse tripped, GPIO Q6
devmem_clear_bit $(scu_addr 90) 28

gpio_export Q6

# System SPI
# Strap 12 must be 0 and Strape 13 must be 1
devmem_clear_bit $(scu_addr 70) 12
devmem_set_bit $(scu_addr 70) 13

# DEBUG_PORT_UART_SEL_BMC_N: GPIOR1(137)
# To use GPIOR1, SCU88[25] must be 0
devmem_clear_bit $(scu_addr 88) 25

gpio_export R1

# DEBUG UART Controls
# 4 signals: DEBUG_UART_SEL_0/1/2 and DEBUG_UART_RX_SEL_N
# GPIOE0 (32), GPIOE1 (33), GPIOE2 (34) and GPIOE3 (35)

# To enable GPIOE0, SCU80[16], SCU8C[12], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 16
devmem_clear_bit $(scu_addr 8C) 12
devmem_clear_bit $(scu_addr 70) 22

gpio_export_out E0

# To enable GPIOE1, SCU80[17], SCU8C[12], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 17

gpio_export_out E1

# To enable GPIOE2, SCU80[18], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 18
devmem_clear_bit $(scu_addr 8C) 13

gpio_export_out E2

# To enable GPIOE3, SCU80[19], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 19

gpio_export_out E3

# Enable GPIOY3: BoardId(Yosemite or Test system)
devmem_clear_bit $(scu_addr a4) 11

# Power LED for Slot#2:
# To use GPIOM0 (96), SCU90[4], SCU90[5], and SCU84[24] must be 0
devmem_clear_bit $(scu_addr 90) 4
devmem_clear_bit $(scu_addr 90) 5
devmem_clear_bit $(scu_addr 84) 24

gpio_set M0 1

# Power LED for Slot#1:
# To use GPIOM1 (97), SCU90[4], SCU90[5], and SCU84[25] must be 0
devmem_clear_bit $(scu_addr 84) 25

gpio_set M1 1

# Power LED for Slot#4:
# To use GPIOM2 (98), SCU90[4], SCU90[5], and SCU84[26] must be 0
devmem_clear_bit $(scu_addr 84) 26

gpio_set M2 1

# Power LED for Slot#3:
# To use GPIOM3 (99), SCU90[4], SCU90[5], and SCU84[27] must be 0
devmem_clear_bit $(scu_addr 84) 27

gpio_set M3 1


# Identify LED for Slot#2:
# To use GPIOF0 (40), SCU80[24] must be 0
devmem_clear_bit $(scu_addr 80) 24

gpio_set F0 1

# Identify LED for Slot#1:
# To use GPIOF1 (41), SCU80[25], SCUA4[12], must be 0
devmem_clear_bit $(scu_addr 80) 25
devmem_clear_bit $(scu_addr A4) 12

gpio_set F1 1

# Identify LED for Slot#4:
# To use GPIOF2 (42), SCU80[26], SCUA4[13],  must be 0
devmem_clear_bit $(scu_addr 80) 26
devmem_clear_bit $(scu_addr A4) 13

gpio_set F2 1

# Identify LED for Slot#3:
# To use GPIOF3 (43), SCU80[27], SCUA4[14], must be 0
devmem_clear_bit $(scu_addr 80) 27
devmem_clear_bit $(scu_addr A4) 14

gpio_set F3 1

# Front Panel Hand Switch GPIO setup
# HAND_SW_ID1: GPIOR2(138)
# To use GPIOR2, SCU88[26] must be 0
devmem_clear_bit $(scu_addr 88) 26

gpio_export R2

# HAND_SW_ID2: GPIOR3(139)
# To use GPIOR3, SCU88[27] must be 0
devmem_clear_bit $(scu_addr 88) 27

gpio_export R3

# HAND_SW_ID4: GPIOR4(140)
# To use GPIOR4, SCU88[28] must be 0
devmem_clear_bit $(scu_addr 88) 28

gpio_export R4


# HAND_SW_ID8: GPIOR5(141)
# To use GPIOR5, SCU88[29] must be 0
devmem_clear_bit $(scu_addr 88) 29

gpio_export R5

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

# LED_POSTCODE_4: GPIOP4 (124)
gpio_set P4 0

# LED_POSTCODE_5: GPIOP5 (125)
gpio_set P5 0

# LED_POSTCODE_6: GPIOP6 (126)
# To use GPIOP6, SCU88[22] must be 0
devmem_clear_bit $(scu_addr 88) 22

gpio_set P6 0

# LED_POSTCODE_7: GPIOP7 (127)
# To use GPIOP7, SCU88[23] must be 0
devmem_clear_bit $(scu_addr 88) 23

gpio_set P7 0

# BMC_READY_N: GPIOG6 (54)
# To use GPIOG6, SCU84[6] must be 0
devmem_clear_bit $(scu_addr 84) 6

gpio_set G6 0

# BMC_RST_BTN_IN_N: GPIOS0 (144)
# To use GPIOS0, SCU8C[0]
devmem_clear_bit $(scu_addr 8c) 0

gpio_export S0

# RESET for all Slots
# RST_SLOT1_SYS_RESET_N: GPIOH1 (57)
# To use GPIOH1, SCU90[6], SCU90[7] must be 0
devmem_clear_bit $(scu_addr 90) 6
devmem_clear_bit $(scu_addr 90) 7

gpio_set H1 1

# RST_SLOT2_SYS_RESET_N: GPIOH0 (56)
# To use GPIOH0, SCU90[6], SCU90[7] must be 0
gpio_set H0 1

# RST_SLOT3_SYS_RESET_N: GPIOH3 (59)
# To use GPIOH3, SCU90[6], SCU90[7] must be 0
gpio_set H3 1

# RST_SLOT4_SYS_RESET_N: GPIOH2 (58)
# To use GPIOH2, SCU90[6], SCU90[7] must be 0
gpio_set H2 1

# 12V_STBY Enable for Slots

# P12V_STBY_SLOT1_EN: GPIOO5 (117)
# To use GPIOO5, SCU88[13] must be 0
devmem_clear_bit $(scu_addr 88) 13

gpio_set O5 1

# P12V_STBY_SLOT2_EN: GPIOO4 (116)
# To use GPIOO4, SCU88[12] must be 0
devmem_clear_bit $(scu_addr 88) 12

gpio_set O4 1

# P12V_STBY_SLOT3_EN: GPIOO7 (119)
# To use GPIOO7, SCU88[15] must be 0
devmem_clear_bit $(scu_addr 88) 15

gpio_set O7 1

# P12V_STBY_SLOT4_EN: GPIOO6 (118)
# To use GPIOO6, SCU88[13] must be 0
devmem_clear_bit $(scu_addr 88) 14

gpio_set O6 1

# PWRGD_P12V_STBY_SLOT1: GPIOP1 (121)
# To use GPIOP1, SCU88[17] must be 0
devmem_clear_bit $(scu_addr 88) 17
gpio_export P1

# PWRGD_P12V_STBY_SLOT2: GPIOP0 (120)
# To use GPIOP0, SCU88[16] must be 0
devmem_clear_bit $(scu_addr 88) 16
gpio_export P0

# PWRGD_P12V_STBY_SLOT3: GPIOP3 (123)
# To use GPIOP3, SCU88[19] must be 0
devmem_clear_bit $(scu_addr 88) 19
gpio_export P3

# PWRGD_P12V_STBY_SLOT4: GPIOP2 (122)
# To use GPIOP2, SCU88[18] must be 0
devmem_clear_bit $(scu_addr 88) 18
gpio_export P2

# Enable the the EXTRST functionality of GPIOB7
devmem_set_bit $(scu_addr 80) 15
devmem_clear_bit $(scu_addr 90) 31
devmem_set_bit $(scu_addr 3c) 3

# Enable GPIO pins: I2C_SLOTx_ALERT_N pins for BIC firmware update
devmem_clear_bit $(scu_addr 88) 2
gpio_export N2
devmem_clear_bit $(scu_addr 88) 3
gpio_export N3
devmem_clear_bit $(scu_addr 88) 4
gpio_export N4
devmem_clear_bit $(scu_addr 88) 5
gpio_export N5



# Yosemite OOM remediation
#   enable kernel panic (force reboot)
echo 1 >> /proc/sys/vm/panic_on_oom
