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

# SLOT1_PRSNT_N, GPIOAA0 (208)
# GPIOAA0(208): SCU90[31], SCUA4[24] shall be 0
devmem_clear_bit $(scu_addr 90) 31
devmem_clear_bit $(scu_addr a4) 24
gpio_export AA0

# SLOT2_PRSNT_N, GPIOAA1 (209)
# GPIOAA1(209): SCU90[31], SCUA4[25] shall be 0
devmem_clear_bit $(scu_addr a4) 25
gpio_export AA1

# SLOT3_PRSNT_N, GPIOAA2 (210)
# GPIOAA2(210): SCU90[31], SCUA4[26] shall be 0
devmem_clear_bit $(scu_addr a4) 26
gpio_export AA2

# SLOT4_PRSNT_N, GPIOAA3 (211)
# GPIOAA3(211): SCU90[31], SCUA4[27] shall be 0
devmem_clear_bit $(scu_addr a4) 27
gpio_export AA3

# SLOT1_PRSNT_B_N, GPIOZ0 (200)
# GPIOZ0(200): SCU90[31], SCUA4[16] shall be 0
devmem_clear_bit $(scu_addr a4) 16
gpio_export Z0

# SLOT2_PRSNT_B_N, GPIOZ1 (201)
# GPIOZ1(201): SCU90[31], SCUA4[17] shall be 0
devmem_clear_bit $(scu_addr a4) 17
gpio_export Z1

# SLOT3_PRSNT_B_N, GPIOZ2 (202)
# GPIOZ2(202): SCU90[31], SCUA4[18] shall be 0
devmem_clear_bit $(scu_addr a4) 18
gpio_export Z2

# SLOT4_PRSNT_B_N, GPIOZ3 (203)
# GPIOZ3(203): SCU90[31], SCUA4[19] shall be 0
devmem_clear_bit $(scu_addr a4) 19
gpio_export Z3

# BMC_PWR_BTN_IN_N, uServer power button in, on GPIO D0(24)
gpio_export D0

# PWR_SLOT1_BTN_N, 1S Server power out, on GPIO D1
# GPIOD1(25): SCU90[1], SCU8C[8], SCU70[21] shall be 0
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 8c) 8
devmem_clear_scu70_bit 21
gpio_set D1 1

# PWR_SLOT2_BTN_N, 1S Server power out, on GPIO D3
# Make sure the Power Control Pin is Set properly
# GPIOD3(27): SCU90[1], SCU8C[9], SCU[21] shall be 0

devmem_clear_bit $(scu_addr 8c) 9

gpio_set D3 1

# PWR_SLOT3_BTN_N, 1S Server power out, on GPIO D5
# GPIOD5(29): SCU90[1], SCU8C[10], and SCU70[21] shall be 0
devmem_clear_bit $(scu_addr 8c) 10

gpio_set D5 1

# PWR_SLOT4_BTN_N, 1S Server power out, on GPIO D7
# GPIOD7(31): SCU90[1], SCU8C[11], and SCU70[21] shall be 0
devmem_clear_bit $(scu_addr 8c) 11

gpio_set D7 1

# SMB_SLOT1_NIC_ALERT_N, alert for 1S Server NIC I2C, GPIO B0
gpio_export B0
# SMB_SLOT2_NIC_ALERT_N, alert for 1S Server NIC I2C, GPIO B1
gpio_export B1
# SMB_SLOT3_NIC_ALERT_N, alert for 1S Server NIC I2C, GPIO B2
gpio_export B2
# SMB_SLOT4_NIC_ALERT_N, alert for 1S Server NIC I2C, GPIO B3
gpio_export B3

# Enable P3V3: GPIOAB1(217)
# To use GPIOAB1 (217), SCU90[31]=0, (SCUA8[1] or SCU94[1:0]) must be 0
devmem_clear_bit $(scu_addr a8) 1
devmem_clear_bit $(scu_addr 94) 0
devmem_clear_bit $(scu_addr 94) 1

gpio_set AB1 1

# BMC_SELF_HW_RST: GPIOAB2(218)
# To use GPIOAB2 (218), SCUA8[2] must be 0
devmem_clear_bit $(scu_addr a8) 2
gpio_set AB2 0

# VGA Mux
# To use GPIOJ2 (74), SCU84[10] must be 0
devmem_clear_bit $(scu_addr 84) 10
# To use GPIOJ3 (75), SCU84[11] must be 0
devmem_clear_bit $(scu_addr 84) 11

gpio_export J2
gpio_export J3

#========================================================================================================#
# Setup GPIOs to Mux Enable: GPIOAB3(219), Channel Select: GPIOE4(36), GPIOE5(37)

# To use GPIOAB3 (219), SCUA8[3] must be 0
devmem_clear_bit $(scu_addr a8) 3

#To use GPIOE4 (36), SCU80[20], SCU8C[14], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 20
devmem_clear_bit $(scu_addr 8C) 14
devmem_clear_scu70_bit 22

# To use GPIOE5 (37), SCU80[21], SCU8C[14], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 21

gpio_export AB3
gpio_export E4
gpio_export E5
#========================================================================================================#

# USB_OC_N, resettable fuse tripped, GPIO Q6
devmem_clear_bit $(scu_addr 2c) 1

gpio_export Q6

# DEBUG_PORT_UART_SEL_BMC_N: GPIOR3(139)
# To use GPIOR3, SCU88[27] must be 0
devmem_clear_bit $(scu_addr 88) 27

gpio_export R3

# DEBUG UART Controls
# 4 signals: DEBUG_UART_SEL_0/1/2 and DEBUG_UART_RX_SEL_N
# GPIOE0 (32), GPIOE1 (33), GPIOE2 (34) and GPIOE3 (35)

# To enable GPIOE0, SCU80[16], SCU8C[12], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 16
devmem_clear_bit $(scu_addr 8C) 12
devmem_clear_scu70_bit 22

gpio_export E0

# To enable GPIOE1, SCU80[17], SCU8C[12], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 17

gpio_export E1

# To enable GPIOE2, SCU80[18], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 18
devmem_clear_bit $(scu_addr 8C) 13

gpio_export E2

# To enable GPIOE3, SCU80[19], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 19

gpio_export E3

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
# To use GPIOF0 (40), SCU80[24], SCU90[30] must be 0
devmem_clear_bit $(scu_addr 80) 24
devmem_clear_bit $(scu_addr 90) 30

gpio_set F0 1

# Identify LED for Slot#1:
# To use GPIOF1 (41), SCU80[25], SCU90[30], must be 0
devmem_clear_bit $(scu_addr 80) 25
gpio_set F1 1

# Identify LED for Slot#4:
# To use GPIOF2 (42), SCU80[26], SCU90[30],  must be 0
devmem_clear_bit $(scu_addr 80) 26
gpio_set F2 1

# Identify LED for Slot#3:
# To use GPIOF3 (43), SCU80[27], SCU90[30], must be 0
devmem_clear_bit $(scu_addr 80) 27
gpio_set F3 1

# Front Panel Hand Switch GPIO setup
# HAND_SW_ID1: GPIOAA4(212)
# To use GPIOAA4, SCUA4[28] must be 0
devmem_clear_bit $(scu_addr a4) 28

gpio_export AA4

# HAND_SW_ID2: GPIOAA5(213)
# To use GPIOAA5,  SCUA4[29] must be 0
devmem_clear_bit $(scu_addr a4) 29

gpio_export AA5

# HAND_SW_ID4: GPIOAA6(214)
# To use GPIOAA6, SCUA4[30] must be 0
devmem_clear_bit $(scu_addr a4) 30

gpio_export AA6

# HAND_SW_ID8: GPIOAA7(215)
# To use GPIOAA7, SCUA4[31] must be 0
devmem_clear_bit $(scu_addr a4) 31

gpio_export AA7


# SLOT1_LED: GPIOAC0(224)
gpio_export AC0

# SLOT2_LED: GPIOAC1(225)
gpio_export AC1

# SLOT3_LED: GPIOAC2(226)
gpio_export AC2

# SLOT4_LED: GPIOAC3(227)
gpio_export AC3

# SLED_SEATED_N: GPIOAC7(231)
gpio_export AC7

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

# DUAL FAN DETECT: GPIOG6 (54)
# To use GPIOG6, SCU84[7] and SCU94[12]  must be 0
devmem_clear_bit $(scu_addr 84) 7
devmem_clear_bit $(scu_addr 94) 12

gpio_export G6

# LED_POSTCODE_4: GPIOP4 (124)
gpio_set P4 0

# LED_POSTCODE_5: GPIOP5 (125)
gpio_set P5 0

# LED_POSTCODE_6: GPIOP6 (126)
gpio_set P6 0

# LED_POSTCODE_7: GPIOP7 (127)
gpio_set P7 0

# BMC_READY_N: GPIOA0 (0)
# To use GPIOA0, SCU80[0] must be 0
devmem_clear_bit $(scu_addr 80) 0
gpio_set A0 0

# BMC_RST_BTN_IN_N: GPIOAB0 (216)
# To use GPIOS0, SCU90[31], (SCUA8[0] or SCU94[1:0]) must be 0
gpio_export AB0

# RESET for all Slots
# RST_SLOT1_SYS_RESET_N: GPIOS0 (144)
# To use GPIOS0, SCU8C[0] must be 0
devmem_clear_bit $(scu_addr 8c) 0

gpio_set S0 1

# RST_SLOT2_SYS_RESET_N: GPIOS1 (145)
# To use GPIOS1, SCU8C[0] must be 0
gpio_set S1 1

# RST_SLOT3_SYS_RESET_N: GPIOS2 (146)
# To use GPIOS2, SCU8C[0] must be 0
gpio_set S2 1

# RST_SLOT4_SYS_RESET_N: GPIOS3 (147)
# To use GPIOS3, SCU8C[0] must be 0
gpio_set S3 1

# 12V_STBY Enable for Slots
# P12V_STBY_SLOT1_EN: GPIOO4 (116)
# To use GPIOO4, SCU88[12] must be 0
devmem_clear_bit $(scu_addr 88) 12
gpio_export O4
if [[ $(is_server_prsnt 1) == "1" && $(is_slot_12v_on 1) != "1" ]]; then
  gpio_set O4 1
fi

# P12V_STBY_SLOT2_EN: GPIOO5 (117)
# To use GPIOO5, SCU88[13] must be 0
devmem_clear_bit $(scu_addr 88) 13
gpio_export O5
if [[ $(is_server_prsnt 2) == "1" && $(is_slot_12v_on 2) != "1" ]]; then
  gpio_set O5 1
fi

# P12V_STBY_SLOT3_EN: GPIOO6 (118)
# To use GPIOO6, SCU88[13] must be 0
devmem_clear_bit $(scu_addr 88) 14
gpio_export O6
if [[ $(is_server_prsnt 3) == "1" && $(is_slot_12v_on 3) != "1" ]]; then
  gpio_set O6 1
fi

# P12V_STBY_SLOT4_EN: GPIOO7 (119)
# To use GPIOO7, SCU88[15] must be 0
devmem_clear_bit $(scu_addr 88) 15
gpio_export O7
if [[ $(is_server_prsnt 4) == "1" && $(is_slot_12v_on 4) != "1" ]]; then
  gpio_set O7 1
fi

# SLOT1_EJECTOR_LATCH_DETECT_N: GPIOP0 (120)
# To use GPIOP0, SCU88[16] must be 0
devmem_clear_bit $(scu_addr 88) 16
gpio_export P0

# SLOT2_EJECTOR_LATCH_DETECT_N: GPIOP1 (121)
# To use GPIOP1, SCU88[17] must be 0
devmem_clear_bit $(scu_addr 88) 17
gpio_export P1

# SLOT3_EJECTOR_LATCH_DETECT_N: GPIOP2 (122)
# To use GPIOP2, SCU88[18] must be 0
devmem_clear_bit $(scu_addr 88) 18
gpio_export P2

# SLOT4_EJECTOR_LATCH_DETECT_N: GPIOP3 (123)
# To use GPIOP3, SCU88[19] must be 0
devmem_clear_bit $(scu_addr 88) 19
gpio_export P3

# FAN_LATCH_DETECT : GPIOH5 (61)
# To use GPIOH5, SCU94[7] and SCU90[7] must be 0
devmem_clear_bit $(scu_addr 90) 7
devmem_clear_bit $(scu_addr 94) 7
gpio_export H5

# Enable GPIO pins: I2C_SLOTx_ALERT_N pins for BIC firmware update
devmem_clear_bit $(scu_addr 88) 2
gpio_export N2
devmem_clear_bit $(scu_addr 88) 3
gpio_export N3
devmem_clear_bit $(scu_addr 88) 4
gpio_export N4
devmem_clear_bit $(scu_addr 88) 5
gpio_export N5

# To use GPIOI0~GPIOI7, SCU70[13:12] must be 0
devmem_clear_scu70_bit 12
devmem_clear_scu70_bit 13

# SLOT1_POWER_EN: GPIOI0 (64)
gpio_export I0
# SLOT2_POWER_EN: GPIOI1 (65)
gpio_export I1
# SLOT3_POWER_EN: GPIOI2 (66)
gpio_export I2
# SLOT4_POWER_EN: GPIOI3 (67)
gpio_export I3

# Set SLOT throttle pin
# BMC_THROTTLE_SLOT1_N: GPIOI4 (68)
gpio_set I4 1
# BMC_THROTTLE_SLOT2_N: GPIOI5 (69)
gpio_set I5 1
# BMC_THROTTLE_SLOT3_N: GPIOI6 (70)
gpio_set I6 1
# BMC_THROTTLE_SLOT4_N: GPIOI7 (71)
gpio_set I7 1

# Set FAN disable pin
# DISABLE_FAN_N: GPIOM4 (100)
gpio_set M4 1

# Set FAST PROCHOT pin (Default Enable)
# FAST_PROCHOT_EN: GPIOR4 (140)
gpio_set R4 1

# PE_BUFF_OE_0_N: GPIOB4 (12)
# To use GPIOB4, SCU70[23] must be 0
devmem_clear_scu70_bit 23
gpio_export B4

# PE_BUFF_OE_1_N: GPIOB5 (13)
#To use GPIOB5, SCU80[13] must be 0
devmem_clear_bit $(scu_addr 80) 13
gpio_export B5

# PE_BUFF_OE_2_N: GPIOB6 (14)
devmem_clear_bit $(scu_addr 80) 14
gpio_export B6

# PE_BUFF_OE_3_N: GPIOB7 (15)
gpio_export B7

# CLK_BUFF1_PWR_EN_N: GPIOJ0 (72)
# To use GPIOJ2 (72), SCU84[8] must be 0
devmem_clear_bit $(scu_addr 84) 8
gpio_export J0

# CLK_BUFF2_PWR_EN_N: GPIOJ1 (73)
# To use GPIOJ3 (73), SCU84[9] must be 0
devmem_clear_bit $(scu_addr 84) 9
gpio_export J1

# Disable PWM reset during external reset
devmem_clear_bit $(scu_addr 9c) 17
# Disable PWM reset during WDT1 reset
devmem_clear_bit 0x1e78501c 17

# Set debounce timer #1 value to 0x179A7B0 ~= 2s
$DEVMEM 0x1e780050 32 0x179A7B0

# Select debounce timer #1 for GPIOZ0~GPIOZ3 and GPIOAA0~GPIOAA3
$DEVMEM 0x1e780194 32 0xF0F00

# Set debounce timer #2 value to 0xBCD3D8 ~= 1s
$DEVMEM 0x1e780054 32 0xBCD3D8

# Select debounce timer #2 for GPIOP0~GPIOP3
$DEVMEM 0x1e780100 32 0xF000000

# enable I2C ctrl reg (SCUA4),
devmem_set_bit $(scu_addr A4) 12 #SCL1 en
devmem_set_bit $(scu_addr A4) 13 #SDA1
devmem_set_bit $(scu_addr A4) 14 #SCL2
devmem_set_bit $(scu_addr A4) 15 #SDA2
