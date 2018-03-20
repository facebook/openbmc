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

# Set up to read the board revision pins, GPIOD1(25), GPIOD3 (27), GPIOD5 (29)
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 70) 21

devmem_clear_bit $(scu_addr 8c) 8
devmem_clear_bit $(scu_addr 8c) 9
devmem_clear_bit $(scu_addr 8c) 10

gpio_export D1
gpio_export D3
gpio_export D5

BOARD_REV=$(( $(gpio_get D1) + ($(gpio_get D3)<<1) + ($(gpio_get D5)<<2) ))

# FM_PWR_BTN_N, Server power button in, on GPIO E2(34)
# To enable GPIOE2, SCU80[18], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 18
devmem_clear_bit $(scu_addr 8C) 13
devmem_clear_bit $(scu_addr 70) 22

gpio_export E2

# FM_BMC_PWRBTN_OUT_N, power button to Server, on GPIO E3(35)
# To enable GPIOE3, SCU80[19], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 19

gpio_set E3 1

# PWRGD_SYS_PWROK, power GOOD , on GPIO B6(14)
# To enable GPIOB6, SCU80[14] must be 0
devmem_clear_bit $(scu_addr 80) 14

gpio_export B6

# LED_POST_CODE_0: GPIOH0 (56)
# To use GPIOH0, SCU90[6], SCU90[7], SCU10[8], SCU94[5] must be 0
devmem_clear_bit $(scu_addr 90) 6
devmem_clear_bit $(scu_addr 90) 7
devmem_clear_bit $(scu_addr 10) 8
devmem_clear_bit $(scu_addr 94) 5

gpio_set H0 1

# LED_POST_CODE_1: GPIOH1 (57)
# To use GPIOH1, SCU90[6], SCU90[7], SCU10[8], SCU94[5]  must be 0
gpio_set H1 1

# LED_POST_CODE_2: GPIOH2 (58)
# To use GPIOH2, SCU90[6], SCU90[7], SCU10[8], SCU94[6]  must be 0
devmem_clear_bit $(scu_addr 94) 6
gpio_set H2 1

# LED_POST_CODE_3: GPIOH3 (59)
# To use GPIOH3, SCU90[6], SCU90[7], SCU10[8], SCU94[6] must be 0
gpio_set H3 1

# LED_POST_CODE_4, GPIOH4 (60)
# GPIOH4(60): SCU90[6], SCU90[7], SCU10[8], SCU94[7] shall be 0
devmem_clear_bit $(scu_addr 94) 7
gpio_set H4 1

#LED_POST_CODE_5, GPIOH5 (61)
# GPIOH5(61): SCU90[6], SCU90[7], SCU10[8], SCU94[7] shall be 0
gpio_set H5 1

# LED_POST_CODE_6, GPIOH6 (62)
# GPIOH6(62): SCU90[6], SCU90[7], SCU10[8] shall be 0
gpio_set H6 1

# LED_POST_CODE_7, GPIOH7 (63)
# GPIOH7(63): SCU90[6], SCU90[7], SCU10[8] shall be 0
gpio_set H7 1

# FM_POST_CARD_PRES_BMC_N: GPIOQ6 (134)
devmem_clear_bit $(scu_addr 90) 28

# Set debounce timer #1 value to 0x12E1FC ~= 100ms
$DEVMEM 0x1e780050 32 0x12E1FC

# Select debounce timer #1 for GPIOQ6
devmem_clear_bit 0x1e780130 6
devmem_set_bit 0x1e780134 6

gpio_export Q6

# BMC_HEARTBEAT_N, heartbeat LED, GPIO U5(165)
# GPIOU5(165): SCUA0[13] shall be 1
devmem_set_bit $(scu_addr A0) 13

gpio_set U5 1

#gpio_set Q7 0

# System SPI
# Strap 12 must be 0 and Strape 13 must be 1
#devmem_clear_bit $(scu_addr 70) 12
#devmem_set_bit $(scu_addr 70) 13

# Power LED for Server:
# To use GPIOAA2 (210), SCUA4[26],  must be 0
devmem_clear_bit $(scu_addr A4) 26

gpio_set AA2 1

# Identify LED for Slot#1:
# To use GPIOF1 (41), SCU80[25], SCUA4[12], must be 0
#devmem_clear_bit $(scu_addr 80) 25
#devmem_clear_bit $(scu_addr A4) 12

#gpio_set F1 1
# Front Panel Hand Switch GPIO setup
# HAND_SW_ID1: GPIOR2(138)
# To use GPIOR2, SCU88[26] must be 0
#devmem_clear_bit $(scu_addr 88) 26

#gpio_export R2

# HAND_SW_ID2: GPIOR3(139)
# To use GPIOR3, SCU88[27] must be 0
#devmem_clear_bit $(scu_addr 88) 27

#gpio_export R3

# HAND_SW_ID4: GPIOR4(140)
# To use GPIOR4, SCU88[28] must be 0
#devmem_clear_bit $(scu_addr 88) 28

#gpio_export R4


# HAND_SW_ID8: GPIOR5(141)
# To use GPIOR5, SCU88[29] must be 0
#devmem_clear_bit $(scu_addr 88) 29

#gpio_export R5

# LED POST CODES: 8 GPIO signals

# LED_POSTCODE_0: GPIOG0 (48)
# To use GPIOG0, SCU84[0] must be 0
#devmem_clear_bit $(scu_addr 84) 0

#gpio_set G0 0

# LED_POSTCODE_1: GPIOG1 (49)
# To use GPIOG1, SCU84[1] must be 0
#devmem_clear_bit $(scu_addr 84) 1

#gpio_set G1 0

# LED_POSTCODE_2: GPIOG2 (50)
# To use GPIOG2, SCU84[2] must be 0
#devmem_clear_bit $(scu_addr 84) 2

#gpio_set G2 0

# LED_POSTCODE_4: GPIOP4 (124)
#gpio_set P4 0

# LED_POSTCODE_5: GPIOP5 (125)
#gpio_set P5 0

# LED_POSTCODE_6: GPIOP6 (126)
# To use GPIOP6, SCU88[22] must be 0
#devmem_clear_bit $(scu_addr 88) 22

#gpio_set P6 0

# LED_POSTCODE_7: GPIOP7 (127)
# To use GPIOP7, SCU88[23] must be 0
#devmem_clear_bit $(scu_addr 88) 23

#gpio_set P7 0

# BMC_READY_N: GPIOS1 (145)
# To use GPIOS1, SCU8C[1] must be 0
devmem_clear_bit $(scu_addr 8C) 1

# Disable GPIO and enable it from
# KCS
gpio_set S1 1

# BMC_RST_BTN_IN_N: GPIOS0 (144)
# To use GPIOS0, SCU8C[0]
#devmem_clear_bit $(scu_addr 8c) 0

#gpio_export S0

# RESET for all Slots
# RST_SLOT1_SYS_RESET_N: GPIOH1 (57)
# To use GPIOH1, SCU90[6], SCU90[7] must be 0
#devmem_clear_bit $(scu_addr 90) 6
#devmem_clear_bit $(scu_addr 90) 7

#gpio_set H1 1

# 12V_STBY Enable for Slots

# P12V_STBY_SLOT1_EN: GPIOO5 (117)
# To use GPIOO5, SCU88[13] must be 0
#devmem_clear_bit $(scu_addr 88) 13

#gpio_set O5 1

# PWRGD_P12V_STBY_SLOT1: GPIOP1 (121)
# To use GPIOP1, SCU88[17] must be 0
#devmem_clear_bit $(scu_addr 88) 17
#gpio_export P1

# Enable the the EXTRST functionality of GPIOB7
#devmem_set_bit $(scu_addr 80) 15
#devmem_clear_bit $(scu_addr 90) 31
#devmem_set_bit $(scu_addr 3c) 3


# Disable PWM reset during external reset
devmem_clear_bit $(scu_addr 9c) 17
# Disable PWM reset during WDT1 reset
devmem_clear_bit 0x1e78501c 17

# FM_BATTERY_SENSE_EN_N: GPIOF6 (46)
# To use GJPIOF6, SCU80[30] must be 0
devmem_clear_bit $(scu_addr 80) 30
gpio_set F6 1

# FM_BIOS_MRC_DEBUG_MSG_DIS_N: GPIOD0 (24)
# To use GJPIOD0, SCU90[1] must be 0
devmem_clear_bit $(scu_addr 90) 1
gpio_set D0 1

# Platform ID check pin
devmem_clear_bit $(scu_addr 88) 16
devmem_clear_bit $(scu_addr 88) 17
devmem_clear_bit $(scu_addr 88) 18
devmem_clear_bit $(scu_addr 88) 19
devmem_clear_bit $(scu_addr 88) 20
devmem_clear_bit $(scu_addr 88) 30
devmem_clear_bit $(scu_addr 88) 31

gpio_export P0
gpio_export P1
gpio_export P2
gpio_export P3
gpio_export P4
gpio_export R6
gpio_export R7

# HSC_SMBUS_SWITCH_EN: GPIOS3 (147)
# To use GPIOS3, SCU8C[3] must be 0
devmem_clear_bit $(scu_addr 8C) 3

gpio_export S3

# FM_FAST_PROCHOT_EN_N: GPIOV3 (171)
# To use GPIOV3, SCUA0[19] must be 1
devmem_set_bit $(scu_addr A0) 19

gpio_set V3 0

# FM_OC_DETECT_EN_N: GPIOM3 (99)
# To use GPIOM3, SCU84[27] must be 0
devmem_clear_bit $(scu_addr 84) 27

gpio_set M3 0

# UV_HIGH_SET: GPIOAA5(???)
# To use GIOAA5, SCUA4[29] must be 0
devmem_clear_bit $(scu_addr A4) 29

gpio_export AA5

# FM_PCH_BMC_THERMTRIP_N: GPIOG2 (50)
# To use GPIOG2, SCU84[2] must be 0
devmem_clear_bit $(scu_addr 84) 2

gpio_export G2

# FM_BIOS_SMI_ACTIVE_N: GPIOG7 (55) 
gpio_export G7

# FM_PMBUS_ALERT_BUF_EN_N: GPIOL5 (93)
# To use GPIOG2, SCU84[21] and SCU90[5] must be 0
devmem_clear_bit $(scu_addr 84) 21
devmem_clear_bit $(scu_addr 90) 5

gpio_set L5 0

# FM_CPU0_PROCHOT_LVT3_ BMC_N, on GPIO E6(38)
# To enable GPIOE6, SCU80[22], SCU8C[15], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 22
devmem_clear_bit $(scu_addr 8C) 15
devmem_clear_bit $(scu_addr 70) 22

gpio_export E6

# FM_CPU1_PROCHOT_LVT3_ BMC_N, on GPIO E7(39)
# To enable GPIOE7, SCU80[23], SCU8C[15], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 23

gpio_export E7

# RST_BMC_SYSRST_BTN_OUT_N, power reset to Server, on GPIO E1(33)
devmem_clear_bit $(scu_addr 80) 17

gpio_set E1 1

# Disable JTAG Engine
devmem_clear_bit $(jtag_addr 08) 30
devmem_clear_bit $(jtag_addr 08) 31

# FM_CPU_MSMI_LVT3_N
devmem_clear_bit $(scu_addr 88) 3
gpio_export N3

# FM_CPU_CATERR_LVT3_N
devmem_clear_bit $(scu_addr 84) 1
gpio_export G1

# FM_GLOBAL_RST_WARN_N
devmem_set_bit $(scu_addr A4) 3
# H_CPU0_MEMABC_MEMHOT_LVT3_BMC_N
devmem_set_bit $(scu_addr A4) 4
# H_CPU0_MEMDEF_MEMHOT_LVT3_BMC_N
devmem_set_bit $(scu_addr A4) 5
# H_CPU1_MEMGHJ_MEMHOT_LVT3_BMC_N
devmem_set_bit $(scu_addr A4) 6
# H_CPU1_MEMKLM_MEMHOT_LVT3_BMC_N
devmem_set_bit $(scu_addr A4) 7

# Intel remote BMC debug function pins
# GPIOP5 BMC_PREQ_N
devmem_clear_bit $(scu_addr 88) 21
gpio_set P5 1
# GPIOP6 BMC_PWR_DEBUG_N
devmem_clear_bit $(scu_addr 88) 22
# gpio_set P6 1

# GPIOP7 RST_RSMRST_N
devmem_clear_bit $(scu_addr 88) 23
gpio_export P7
# GPIOB1 BMC_DEBUG_EN_N
gpio_export B1
# GPIOR2 BMC_TCK_MUX_SEL
devmem_clear_bit $(scu_addr 88) 26
gpio_export R2
# GPIOR3 BMC_PRDY_N
devmem_clear_bit $(scu_addr 88) 27
gpio_export R3
# GPIOR4 BMC_XDP_PRSNT_IN_N
devmem_clear_bit $(scu_addr 88) 28
gpio_export R4
# GPIOR5 RST_BMC_PLTRST_BUF_N
devmem_clear_bit $(scu_addr 88) 29
gpio_export R5
# GPIOY2 BMC_JTAG_SEL
devmem_clear_bit $(scu_addr A4) 10
devmem_clear_bit $(scu_addr 94) 11
gpio_set Y2 1

# FM_CPU0_SKTOCC_LVT3_N
devmem_clear_bit $(scu_addr 84) 3
gpio_export G3

# FM_CPU1_SKTOCC_LVT3_N
devmem_clear_bit $(scu_addr A4) 24
gpio_export AA0

# FM_BIOS_POST_CMPLT_N
devmem_clear_bit $(scu_addr A4) 31
gpio_export AA7

# PECI_MUX_SELECT
if [ $BOARD_REV -le 2 ]; then
  # Power-On, EVT, preDVT
  devmem_clear_bit $(scu_addr A8) 2
  gpio_set AB2 0
else
  # DVT or newer
  devmem_clear_bit $(scu_addr A4) 28
  gpio_set AA4 0
fi

# BMC_CPLD_FPGA_SEL: GPIOA0
devmem_clear_bit $(scu_addr 80) 0
gpio_set A0 1

# FM_SLPS4_N: GPIOY1
devmem_clear_bit $(scu_addr A4) 9
devmem_clear_bit $(scu_addr 94) 10
gpio_export Y1

# FW_UV_ADR_TRIGGER_EN: GPIOI3
gpio_set I3 1

# FW_FORCE_ADR_N: GPIOI2
gpio_set I2 1

# IRQ_DIMM_SAVE_LVT3_N
gpio_export D4

# FM_OCP_MEZZA_PRES: GPIOAB1
devmem_clear_bit $(scu_addr A8) 1
gpio_export AB1

# FM_UARTSW_MSB_N:
devmem_clear_bit $(scu_addr 90) 27

gpio_export Q4
gpio_export Q5

#PPIN
devmem_clear_bit $(scu_addr 80) 13
gpio_export B5
gpio_set B5 1
