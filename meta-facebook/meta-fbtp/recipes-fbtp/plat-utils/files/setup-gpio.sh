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

# Set up to read the board revision pins, GPIOD1(25), GPIOD3 (27), GPIOD5 (29)
devmem_clear_bit $(scu_addr 90) 1
devmem_clear_bit $(scu_addr 70) 21

devmem_clear_bit $(scu_addr 8c) 8
devmem_clear_bit $(scu_addr 8c) 9
devmem_clear_bit $(scu_addr 8c) 10

gpio_export FM_BOARD_REV_ID0 GPIOD1
gpio_export FM_BOARD_REV_ID1 GPIOD3
gpio_export FM_BOARD_REV_ID2 GPIOD5

BOARD_REV=$(( $(gpio_get FM_BOARD_REV_ID0) + ($(gpio_get FM_BOARD_REV_ID1)<<1) + ($(gpio_get FM_BOARD_REV_ID2)<<2) ))

# FM_PWR_BTN_N, Server power button in, on GPIO E2(34)
# To enable GPIOE2, SCU80[18], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 18
devmem_clear_bit $(scu_addr 8C) 13
devmem_clear_bit $(scu_addr 70) 22

gpio_export FM_PWR_BTN_N GPIOE2

# FM_BMC_PWRBTN_OUT_N, power button to Server, on GPIO E3(35)
# To enable GPIOE3, SCU80[19], SCU8C[13], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 19
gpio_export FM_BMC_PWRBTN_OUT_N GPIOE3
gpio_set FM_BMC_PWRBTN_OUT_N 1

# PWRGD_SYS_PWROK, power GOOD , on GPIO B6(14)
# To enable GPIOB6, SCU80[14] must be 0
devmem_clear_bit $(scu_addr 80) 14
gpio_export PWRGD_SYS_PWROK GPIOB6

# LED_POST_CODE_0: GPIOH0 (56)
# To use GPIOH0, SCU90[6], SCU90[7], SCU10[8], SCU94[5] must be 0
devmem_clear_bit $(scu_addr 90) 6
devmem_clear_bit $(scu_addr 90) 7
devmem_clear_bit $(scu_addr 10) 8
devmem_clear_bit $(scu_addr 94) 5

gpio_export LED_POST_CODE_0 GPIOH0
gpio_set LED_POST_CODE_0 1

# LED_POST_CODE_1: GPIOH1 (57)
# To use GPIOH1, SCU90[6], SCU90[7], SCU10[8], SCU94[5]  must be 0
gpio_export LED_POST_CODE_1 GPIOH1
gpio_set LED_POST_CODE_1 1

# LED_POST_CODE_2: GPIOH2 (58)
# To use GPIOH2, SCU90[6], SCU90[7], SCU10[8], SCU94[6]  must be 0
devmem_clear_bit $(scu_addr 94) 6
gpio_export LED_POST_CODE_2 GPIOH2
gpio_set LED_POST_CODE_2 1

# LED_POST_CODE_3: GPIOH3 (59)
# To use GPIOH3, SCU90[6], SCU90[7], SCU10[8], SCU94[6] must be 0
gpio_export LED_POST_CODE_3 GPIOH3
gpio_set LED_POST_CODE_3 1

# LED_POST_CODE_4, GPIOH4 (60)
# GPIOH4(60): SCU90[6], SCU90[7], SCU10[8], SCU94[7] shall be 0
devmem_clear_bit $(scu_addr 94) 7
gpio_export LED_POST_CODE_4 GPIOH4
gpio_set LED_POST_CODE_4 1

#LED_POST_CODE_5, GPIOH5 (61)
# GPIOH5(61): SCU90[6], SCU90[7], SCU10[8], SCU94[7] shall be 0
gpio_export LED_POST_CODE_5 GPIOH5
gpio_set LED_POST_CODE_5 1

# LED_POST_CODE_6, GPIOH6 (62)
# GPIOH6(62): SCU90[6], SCU90[7], SCU10[8] shall be 0
gpio_export LED_POST_CODE_6 GPIOH6
gpio_set LED_POST_CODE_6 1

# LED_POST_CODE_7, GPIOH7 (63)
# GPIOH7(63): SCU90[6], SCU90[7], SCU10[8] shall be 0
gpio_export LED_POST_CODE_7 GPIOH7
gpio_set LED_POST_CODE_7 1

# FM_POST_CARD_PRES_BMC_N: GPIOQ6 (134)
devmem_clear_bit $(scu_addr 90) 28

# Set debounce timer #1 value to 0x12E1FC ~= 100ms
$DEVMEM 0x1e780050 32 0x12E1FC

# Select debounce timer #1 for GPIOQ6
devmem_clear_bit 0x1e780130 6
devmem_set_bit 0x1e780134 6

gpio_export FM_POST_CARD_PRES_BMC_N GPIOQ6

# BMC_FAULT_N, fault LED, GPIO U5(165)
# GPIOU5(165): SCUA0[13] shall be 1
devmem_set_bit $(scu_addr A0) 13
gpio_export BMC_FAULT_N GPIOU5
gpio_set BMC_FAULT_N 1

#gpio_set Q7 0 # FM_PE_BMC_WAKE_N

# System SPI
# Strap 12 must be 0 and Strape 13 must be 1
#devmem_clear_bit $(scu_addr 70) 12
#devmem_set_bit $(scu_addr 70) 13

# Power LED for Server:
# To use GPIOAA2 (210), SCUA4[26],  must be 0
devmem_clear_bit $(scu_addr A4) 26
gpio_export SERVER_POWER_LED GPIOAA2  # Confirm Shadow name
gpio_set SERVER_POWER_LED 1

# BMC_READY_N: GPIOS1 (145)
# To use GPIOS1, SCU8C[1] must be 0
devmem_clear_bit $(scu_addr 8C) 1
gpio_export BMC_READY_N GPIOS1
# Disable GPIO and enable it from KCS
gpio_set BMC_READY_N 1

# Disable PWM reset during external reset
devmem_clear_bit $(scu_addr 9c) 17
# Disable PWM reset during WDT1 reset
devmem_clear_bit 0x1e78501c 17

# FM_BATTERY_SENSE_EN_N: GPIOF6 (46)
# To use GJPIOF6, SCU80[30] must be 0
devmem_clear_bit $(scu_addr 80) 30
gpio_export FM_BATTERY_SENSE_EN_N GPIOF6
gpio_set FM_BATTERY_SENSE_EN_N 1

# FM_BIOS_MRC_DEBUG_MSG_DIS_N: GPIOD0 (24)
# To use GJPIOD0, SCU90[1] must be 0
devmem_clear_bit $(scu_addr 90) 1
gpio_export FM_BIOS_MRC_DEBUG_MSG_DIS_N GPIOD0
gpio_set FM_BIOS_MRC_DEBUG_MSG_DIS_N 1

# Platform ID check pin
devmem_clear_bit $(scu_addr 88) 16
devmem_clear_bit $(scu_addr 88) 17
devmem_clear_bit $(scu_addr 88) 18
devmem_clear_bit $(scu_addr 88) 19
devmem_clear_bit $(scu_addr 88) 20
devmem_clear_bit $(scu_addr 88) 30
devmem_clear_bit $(scu_addr 88) 31
gpio_export FM_BOARD_SKU_ID0 GPIOP0
gpio_export FM_BOARD_SKU_ID1 GPIOP1
gpio_export FM_BOARD_SKU_ID2 GPIOP2
gpio_export FM_BOARD_SKU_ID3 GPIOP3
gpio_export FM_BOARD_SKU_ID4 GPIOP4
gpio_export FM_BOARD_SKU_ID5 GPIOR6
gpio_export FM_BOARD_SKU_ID6 GPIOR7

# HSC_SMBUS_SWITCH_EN: GPIOS3 (147)
# To use GPIOS3, SCU8C[3] must be 0
devmem_clear_bit $(scu_addr 8C) 3

gpio_export HSC_SMBUS_SWITCH_EN GPIOS3

# FM_FAST_PROCHOT_EN_N: GPIOV3 (171)
# To use GPIOV3, SCUA0[19] must be 1
devmem_set_bit $(scu_addr A0) 19
gpio_export FM_FAST_PROCHOT_EN_N GPIOV3
gpio_set FM_FAST_PROCHOT_EN_N 0

# FM_OC_DETECT_EN_N: GPIOM3 (99)
# To use GPIOM3, SCU84[27] must be 0
devmem_clear_bit $(scu_addr 84) 27
gpio_export FM_OC_DETECT_EN_N GPIOM3
gpio_set FM_OC_DETECT_EN_N 0

# UV_HIGH_SET: GPIOAA5(???)
# To use GIOAA5, SCUA4[29] must be 0
devmem_clear_bit $(scu_addr A4) 29
gpio_export UV_HIGH_SET GPIOAA5

# FM_PCH_BMC_THERMTRIP_N: GPIOG2 (50)
# To use GPIOG2, SCU84[2] must be 0
devmem_clear_bit $(scu_addr 84) 2
gpio_export FM_PCH_BMC_THERMTRIP_N GPIOG2

# FM_BIOS_SMI_ACTIVE_N: GPIOG7 (55) 
gpio_export FM_BIOS_SMI_ACTIVE_N GPIOG7

# FM_PMBUS_ALERT_BUF_EN_N: GPIOL5 (93)
# To use GPIOG2, SCU84[21] and SCU90[5] must be 0
devmem_clear_bit $(scu_addr 84) 21
devmem_clear_bit $(scu_addr 90) 5
gpio_export FM_PMBUS_ALERT_BUF_EN_N GPIOL5
gpio_set FM_PMBUS_ALERT_BUF_EN_N 0

# FM_CPU0_PROCHOT_LVT3_ BMC_N, on GPIO E6(38)
# To enable GPIOE6, SCU80[22], SCU8C[15], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 22
devmem_clear_bit $(scu_addr 8C) 15
devmem_clear_bit $(scu_addr 70) 22
gpio_export FM_CPU0_PROCHOT_LVT3_BMC_N GPIOE6

# FM_CPU1_PROCHOT_LVT3_ BMC_N, on GPIO E7(39)
# To enable GPIOE7, SCU80[23], SCU8C[15], and SCU70[22] must be 0
devmem_clear_bit $(scu_addr 80) 23
gpio_export FM_CPU1_PROCHOT_LVT3_BMC_N GPIOE7

# RST_BMC_SYSRST_BTN_OUT_N, power reset to Server, on GPIO E1(33)
devmem_clear_bit $(scu_addr 80) 17
gpio_export RST_BMC_SYSRST_BTN_OUT_N GPIOE1
gpio_set RST_BMC_SYSRST_BTN_OUT_N 1

# Disable JTAG Engine
devmem_clear_bit $(jtag_addr 08) 30
devmem_clear_bit $(jtag_addr 08) 31

# FM_CPU_MSMI_LVT3_N
devmem_clear_bit $(scu_addr 88) 3
gpio_export FM_CPU_MSMI_LVT3_N GPION3

# FM_CPU_CATERR_LVT3_N
devmem_clear_bit $(scu_addr 84) 1
gpio_export FM_CPU_CATERR_LVT3_N GPIOG1

# FM_GLOBAL_RST_WARN_N
devmem_set_bit $(scu_addr A4) 3
gpio_export FM_GLOBAL_RST_WARN_N GPIOX3
# H_CPU0_MEMABC_MEMHOT_LVT3_BMC_N
devmem_set_bit $(scu_addr A4) 4
gpio_export H_CPU0_MEMABC_MEMHOT_LVT3_BMC_N GPIOX4
# H_CPU0_MEMDEF_MEMHOT_LVT3_BMC_N
devmem_set_bit $(scu_addr A4) 5
gpio_export H_CPU0_MEMDEF_MEMHOT_LVT3_BMC_N GPIOX5
# H_CPU1_MEMGHJ_MEMHOT_LVT3_BMC_N
devmem_set_bit $(scu_addr A4) 6
gpio_export H_CPU1_MEMGHJ_MEMHOT_LVT3_BMC_N GPIOX6
# H_CPU1_MEMKLM_MEMHOT_LVT3_BMC_N
devmem_set_bit $(scu_addr A4) 7
gpio_export H_CPU1_MEMKLM_MEMHOT_LVT3_BMC_N GPIOX7

# Intel remote BMC debug function pins
# GPIOP5 BMC_PREQ_N
devmem_clear_bit $(scu_addr 88) 21
gpio_export BMC_PREQ_N GPIOP5
gpio_set BMC_PREQ_N 1

# GPIOP6 BMC_PWR_DEBUG_N
devmem_clear_bit $(scu_addr 88) 22
gpio_export BMC_PWR_DEBUG_N GPIOP6
# gpio_set BMC_PWR_DEBUG_N 1

# GPIOP7 RST_RSMRST_N
devmem_clear_bit $(scu_addr 88) 23
gpio_export RST_RSMRST_N GPIOP7

# GPIOB1 BMC_DEBUG_EN_N
gpio_export BMC_DEBUG_EN_N GPIOB1

# GPIOR2 BMC_TCK_MUX_SEL
devmem_clear_bit $(scu_addr 88) 26
gpio_export BMC_TCK_MUX_SEL GPIOR2

# GPIOR3 BMC_PRDY_N
devmem_clear_bit $(scu_addr 88) 27
gpio_export BMC_PRDY_N GPIOR3

# GPIOR4 BMC_XDP_PRSNT_IN_N
devmem_clear_bit $(scu_addr 88) 28
gpio_export BMC_XDP_PRSNT_IN_N GPIOR4

# GPIOR5 RST_BMC_PLTRST_BUF_N
devmem_clear_bit $(scu_addr 88) 29
gpio_export RST_BMC_PLTRST_BUF_N GPIOR5

# GPIOY2 BMC_JTAG_SEL
devmem_clear_bit $(scu_addr A4) 10
devmem_clear_bit $(scu_addr 94) 11
gpio_export BMC_JTAG_SEL GPIOY2
gpio_set BMC_JTAG_SEL 1

# FM_CPU0_SKTOCC_LVT3_N
devmem_clear_bit $(scu_addr 84) 3
gpio_export FM_CPU0_SKTOCC_LVT3_N GPIOG3

# FM_CPU1_SKTOCC_LVT3_N
devmem_clear_bit $(scu_addr A4) 24
gpio_export FM_CPU1_SKTOCC_LVT3_N GPIOAA0

# FM_BIOS_POST_CMPLT_N
devmem_clear_bit $(scu_addr A4) 31
gpio_export FM_BIOS_POST_CMPLT_N GPIOAA7

# PECI_MUX_SELECT
if [ $BOARD_REV -le 2 ]; then
  # Power-On, EVT, preDVT
  devmem_clear_bit $(scu_addr A8) 2
  gpio_export PECI_MUX_SELECT GPIOAB2
else
  # DVT or newer
  devmem_clear_bit $(scu_addr A4) 28
  gpio_export PECI_MUX_SELECT GPIOAA4
fi
gpio_set PECI_MUX_SELECT 0

# BMC_CPLD_FPGA_SEL: GPIOA0
devmem_clear_bit $(scu_addr 80) 0
gpio_export BMC_CPLD_FPGA_SEL GPIOA0
gpio_set BMC_CPLD_FPGA_SEL 1

# FM_SLPS4_N: GPIOY1
devmem_clear_bit $(scu_addr A4) 9
devmem_clear_bit $(scu_addr 94) 10
gpio_export FM_SLPS4_N GPIOY1

# FW_UV_ADR_TRIGGER_EN: GPIOI3
gpio_export FM_UV_ADR_TRIGGER_EN GPIOI3
gpio_set FM_UV_ADR_TRIGGER_EN 1

# FW_FORCE_ADR_N: GPIOI2
gpio_export FM_FORCE_ADR_N GPIOI2
gpio_set FM_FORCE_ADR_N 1

# IRQ_DIMM_SAVE_LVT3_N
gpio_export IRQ_DIMM_SAVE_LVT3_N GPIOD4

# FM_OCP_MEZZA_PRES: GPIOAB1
devmem_clear_bit $(scu_addr A8) 1
gpio_export FM_OCP_MEZZA_PRES GPIOAB1

# FM_UARTSW_MSB_N:
# MSB:LSB
# 1:1 - 14pin,3.0 no connect.
# 1:0 - 14pin,3.0 connect midplane
# 0:1 - 14pin,3.0 connect BMC
# 0:0 - 14pin,3.0 connect PCH
devmem_clear_bit $(scu_addr 90) 27
gpio_export FM_UARTSW_LSB_N GPIOQ4
gpio_export FM_UARTSW_MSB_N GPIOQ5

#PPIN
devmem_clear_bit $(scu_addr 80) 13
gpio_export BMC_PPIN GPIOB5
gpio_set BMC_PPIN 1

# IRQ_PVDDQ_GHJ_VRHOT_LVT3_N
gpio_export IRQ_PVDDQ_GHJ_VRHOT_LVT3_N GPIOB7

# FM_CPU_ERR0_LVT3_BMC_N
gpio_export FM_CPU_ERR0_LVT3_BMC_N GPIOD6

# FM_CPU_ERR1_LVT3_BMC_N
gpio_export FM_CPU_ERR1_LVT3_BMC_N GPIOD7

# RST_SYSTEM_BTN_N
gpio_export RST_SYSTEM_BTN_N GPIOE0

# FP_NMI_BTN_N
gpio_export FP_NMI_BTN_N GPIOE4

# IRQ_PVDDQ_ABC_VRHOT_LVT3_N
gpio_export IRQ_PVDDQ_ABC_VRHOT_LVT3_N GPIOF0

# IRQ_PVCCIN_CPU0_VRHOT_LVC3_N
gpio_export IRQ_PVCCIN_CPU0_VRHOT_LVC3_N GPIOF2

# IRQ_PVCCIN_CPU1_VRHOT_LVC3_N
gpio_export IRQ_PVCCIN_CPU1_VRHOT_LVC3_N GPIOF3

# IRQ_PVDDQ_KLM_VRHOT_LVT3_N
gpio_export IRQ_PVDDQ_KLM_VRHOT_LVT3_N GPIOF4

# FM_CPU_ERR2_LVT3_N
gpio_export FM_CPU_ERR2_LVT3_N GPIOG0

# FM_CPU0_FIVR_FAULT_LVT3_N
gpio_export FM_CPU0_FIVR_FAULT_LVT3_N GPIOI0

# FM_CPU1_FIVR_FAULT_LVT3_N
gpio_export FM_CPU1_FIVR_FAULT_LVT3_N GPIOI1

# IRQ_UV_DETECT_N
gpio_export IRQ_UV_DETECT_N GPIOL0

# IRQ_OC_DETECT_N
gpio_export IRQ_OC_DETECT_N GPIOL1

# FM_HSC_TIMER_EXP_N
gpio_export FM_HSC_TIMER_EXP_N GPIOL2

# FM_MEM_THERM_EVENT_PCH_N
gpio_export FM_MEM_THERM_EVENT_PCH_N GPIOL4

# FM_CPU0_RC_ERROR_N
gpio_export FM_CPU0_RC_ERROR_N GPIOM0

# FM_CPU1_RC_ERROR_N
gpio_export FM_CPU1_RC_ERROR_N GPIOM1

# FM_CPU0_THERMTRIP_LATCH_LVT3_N
gpio_export FM_CPU0_THERMTRIP_LATCH_LVT3_N GPIOM4

# FM_CPU1_THERMTRIP_LATCH_LVT3_N
gpio_export FM_CPU1_THERMTRIP_LATCH_LVT3_N GPIOM5

# FM_THROTTLE_N
gpio_export FM_THROTTLE_N GPIOS0

# IRQ_PVDDQ_DEF_VRHOT_LVT3_N
gpio_export IRQ_PVDDQ_DEF_VRHOT_LVT3_N GPIOZ2

# IRQ_SML1_PMBUS_ALERT_N
gpio_export IRQ_SML1_PMBUS_ALERT_N GPIOAA1

# IRQ_HSC_FAULT_N
gpio_export IRQ_HSC_FAULT_N GPIOAB0

# BMC_BIOS_FLASH_CTRL
gpio_export BMC_BIOS_FLASH_CTRL GPION5
