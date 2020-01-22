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

# FM_BOARD_BMC_REV_ID0
gpio_export FM_BOARD_BMC_REV_ID0 GPIOA0

# FM_BOARD_BMC_REV_ID1 / FM_CPU0_THERMTRIP_LVT3_PLD_N
gpio_export FM_CPU0_THERMTRIP_LVT3_PLD_N GPIOA1
ln -n /tmp/gpionames/FM_CPU0_THERMTRIP_LVT3_PLD_N /tmp/gpionames/FM_BOARD_BMC_REV_ID1

# FM_SPD_DDRCPU_LVLSHFT_EN
gpio_export FM_SPD_DDRCPU_LVLSHFT_EN GPIOA3

# SMB_SLOT1_ALT_N
gpio_export SMB_SLOT1_ALT_N GPIOA6

# SMB_SLOT2_ALT_N
gpio_export SMB_SLOT2_ALT_N GPIOA7

# To enable GPIOB
#devmem_clear_scu70_bit 23
#devmem_clear_bit $(scu_addr 80) 13
#devmem_clear_bit $(scu_addr 80) 14

# FM_BOARD_BMC_SKU_ID0 / FM_MEM_THERM_EVENT_CPU0_LVT3_N
gpio_export FM_MEM_THERM_EVENT_CPU0_LVT3_N GPIOB0
ln -n /tmp/gpionames/FM_MEM_THERM_EVENT_CPU0_LVT3_N /tmp/gpionames/FM_BOARD_BMC_SKU_ID0

# FM_BOARD_BMC_SKU_ID1 / FM_MEM_THERM_EVENT_CPU1_LVT3_N
gpio_export FM_MEM_THERM_EVENT_CPU1_LVT3_N GPIOB1
ln -n /tmp/gpionames/FM_MEM_THERM_EVENT_CPU1_LVT3_N /tmp/gpionames/FM_BOARD_BMC_SKU_ID1

# FM_BOARD_BMC_SKU_ID2 / FM_CPU0_FIVR_FAULT_LVT3_PLD
gpio_export FM_CPU0_FIVR_FAULT_LVT3_PLD GPIOB2
ln -n /tmp/gpionames/FM_CPU0_FIVR_FAULT_LVT3_PLD /tmp/gpionames/FM_BOARD_BMC_SKU_ID2

# FM_BOARD_BMC_SKU_ID3 / FM_CPU1_FIVR_FAULT_LVT3_PLD
gpio_export FM_CPU1_FIVR_FAULT_LVT3_PLD GPIOB3
ln -n /tmp/gpionames/FM_CPU1_FIVR_FAULT_LVT3_PLD /tmp/gpionames/FM_BOARD_BMC_SKU_ID3

# FM_BOARD_BMC_SKU_ID4
gpio_export FM_BOARD_BMC_SKU_ID4 GPIOB4

# FM_BOARD_BMC_SKU_ID5
gpio_export FM_BOARD_BMC_SKU_ID5 GPIOB5

# FM_CPU0_PROCHOT_LVT3_BMC_N
gpio_export FM_CPU0_PROCHOT_LVT3_BMC_N GPIOB6

# FM_CPU1_PROCHOT_LVT3_BMC_N
gpio_export FM_CPU1_PROCHOT_LVT3_BMC_N GPIOB7

# To enable GPIOD
#devmem_clear_scu70_bit 21
#devmem_clear_bit $(scu_addr 8c) 8
#devmem_clear_bit $(scu_addr 8c) 9
#devmem_clear_bit $(scu_addr 8c) 10
#devmem_clear_bit $(scu_addr 8c) 11
#devmem_clear_bit $(scu_addr 90) 1

# FM_BOARD_BMC_REV_ID2 / FM_CPU1_THERMTRIP_LVT3_PLD_N
gpio_export FM_CPU1_THERMTRIP_LVT3_PLD_N GPIOD0
ln -n /tmp/gpionames/FM_CPU1_THERMTRIP_LVT3_PLD_N /tmp/gpionames/FM_BOARD_BMC_REV_ID2

# IRQ_TPM_BMC_SPI_N
gpio_export IRQ_TPM_BMC_SPI_N GPIOD1

# SMB_BMC_ADM1278_ALT_N
gpio_export SMB_BMC_ADM1278_ALT_N GPIOD6

# SMB_BMC_ADM1278_RST_N
gpio_export SMB_BMC_ADM1278_RST_N GPIOD7

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

# FP_BMC_RST_BTN_N
gpio_export FP_BMC_RST_BTN_N GPIOE0

# RST_BMC_RSTBTN_OUT_R_N, power reset to PCH
gpio_export RST_BMC_RSTBTN_OUT_R_N GPIOE1
gpio_set RST_BMC_RSTBTN_OUT_R_N 1

# FM_BMC_PWR_BTN_R_N
gpio_export FM_BMC_PWR_BTN_R_N GPIOE2

# FM_BMC_PWRBTN_OUT_R_N, power button to CPLD
gpio_export FM_BMC_PWRBTN_OUT_R_N GPIOE3
gpio_set FM_BMC_PWRBTN_OUT_R_N 1

# FP_NMI_BTN_N
gpio_export FP_NMI_BTN_N GPIOE4

# IRQ_BMC_PCH_NMI
gpio_export IRQ_BMC_PCH_NMI GPIOE5

# FP_FAULT_LED_N
gpio_export FP_FAULT_LED_N GPIOE6
gpio_set FP_FAULT_LED_N 1

# FP_ID_LED_N, Power LED
gpio_export FP_ID_LED_N GPIOE7
gpio_set FP_ID_LED_N 0

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

# FM_CPU_ERR0_LVT3_N
gpio_export FM_CPU_ERR0_LVT3_N GPIOF0

# FM_CPU_ERR1_LVT3_N
gpio_export FM_CPU_ERR1_LVT3_N GPIOF1

# FM_CPU_ERR2_LVT3_N
gpio_export FM_CPU_ERR2_LVT3_N GPIOF2

# FM_BMC_DEBUG_ENABLE_N
gpio_export FM_BMC_DEBUG_ENABLE_N GPIOF3
gpio_set FM_BMC_DEBUG_ENABLE_N 1

# FM_BMC_UV_ADR_TRIGGER_EN
gpio_export FM_BMC_UV_ADR_TRIGGER_EN GPIOF4

# FM_BMC_FORCE_ADR_N
gpio_export FM_BMC_FORCE_ADR_N GPIOF5

# RST_PLTRST_BMC_N
gpio_export RST_PLTRST_BMC_N GPIOF6

# IRQ_BMC_PRDY_N
gpio_export IRQ_BMC_PRDY_N GPIOF7

# To enable GPIOG
#devmem_clear_bit $(scu_addr 84) 0
#devmem_clear_bit $(scu_addr 84) 1
#devmem_clear_bit $(scu_addr 84) 2
#devmem_clear_bit $(scu_addr 84) 3
#devmem_clear_bit $(scu_addr 84) 4
#devmem_clear_bit $(scu_addr 84) 5
#devmem_clear_bit $(scu_addr 84) 6
#devmem_clear_bit $(scu_addr 94) 12

# FM_CPU1_DISABLE_COD_N
gpio_export FM_CPU1_DISABLE_COD_N GPIOG0

# FM_CPU_MSMI_CATERR_LVT3_N
gpio_export FM_CPU_MSMI_CATERR_LVT3_N GPIOG1

# FM_PCH_BMC_THERMTRIP_N
gpio_export FM_PCH_BMC_THERMTRIP_N GPIOG2

# BMC_READY_N
gpio_export BMC_READY_N GPIOG3
gpio_set BMC_READY_N 1
# To disable Reset Tolerant
devmem_clear_bit 0x1e78003c 19

# IRQ_NMI_EVENT_N
gpio_export IRQ_NMI_EVENT_N GPIOG4

# IRQ_BMC_PCH_SMI_LPC_N
gpio_export IRQ_BMC_PCH_SMI_LPC_N GPIOG5

# IRQ_SMB3_M2_ALERT_N
gpio_export IRQ_SMB3_M2_ALERT_N GPIOG6

# To enable GPIOH
#devmem_clear_bit $(scu_addr 90) 7
#devmem_clear_bit $(scu_addr 94) 5
#devmem_clear_bit $(scu_addr 94) 6
#devmem_clear_bit $(scu_addr 94) 7

# LED_POSTCODE_0
gpio_export LED_POSTCODE_0 GPIOH0
gpio_set LED_POSTCODE_0 0

# LED_POSTCODE_1
gpio_export LED_POSTCODE_1 GPIOH1
gpio_set LED_POSTCODE_1 0

# LED_POSTCODE_2
gpio_export LED_POSTCODE_2 GPIOH2
gpio_set LED_POSTCODE_2 0

# LED_POSTCODE_3
gpio_export LED_POSTCODE_3 GPIOH3
gpio_set LED_POSTCODE_3 0

# LED_POSTCODE_4
gpio_export LED_POSTCODE_4 GPIOH4
gpio_set LED_POSTCODE_4 0

# LED_POSTCODE_5
gpio_export LED_POSTCODE_5 GPIOH5
gpio_set LED_POSTCODE_5 0

# LED_POSTCODE_6
gpio_export LED_POSTCODE_6 GPIOH6
gpio_set LED_POSTCODE_6 0

# LED_POSTCODE_7
gpio_export LED_POSTCODE_7 GPIOH7
gpio_set LED_POSTCODE_7 0

devmem_set_bit 0x1e780068 24
devmem_clear_bit 0x1e78006c 24

# To enable GPIOI
#devmem_clear_scu70_bit 13

# PRSNT_PCIE_CABLE_1_N
gpio_export PRSNT_PCIE_CABLE_1_N GPIOI0

# PRSNT_PCIE_CABLE_2_N
gpio_export PRSNT_PCIE_CABLE_2_N GPIOI1

# FM_PFR_ACTIVE_N
gpio_export FM_PFR_ACTIVE_N GPIOI3

# To enable GPIOL
#devmem_clear_bit $(scu_addr 84) 16
#devmem_clear_bit $(scu_addr 84) 17
#devmem_clear_bit $(scu_addr 84) 18
#devmem_clear_bit $(scu_addr 84) 19
#devmem_clear_bit $(scu_addr 84) 20
#devmem_clear_bit $(scu_addr 84) 21
#devmem_clear_bit $(scu_addr 90) 5

# FM_UARTSW_LSB_N
gpio_export FM_UARTSW_LSB_N GPIOL0

# FM_UARTSW_MSB_N
gpio_export FM_UARTSW_MSB_N GPIOL1

# IRQ_HSC_FAULT_N
gpio_export IRQ_HSC_FAULT_N GPIOL2

# FM_CPU1_MEMHOT_OUT_N
gpio_export FM_CPU1_MEMHOT_OUT_N GPIOL3

# FM_MASTER_MB_N
gpio_export FM_MASTER_MB_N GPIOL4
gpio_set FM_MASTER_MB_N 1

# JTAG_MUX_EN_N
gpio_export JTAG_MUX_EN_N GPIOL5

# To enable GPIOM
#devmem_clear_bit $(scu_addr 84) 24
#devmem_clear_bit $(scu_addr 84) 25
#devmem_clear_bit $(scu_addr 84) 26
#devmem_clear_bit $(scu_addr 84) 27
#devmem_clear_bit $(scu_addr 84) 28
#devmem_clear_bit $(scu_addr 84) 29

# IRQ_OC_DETECT_N
gpio_export IRQ_OC_DETECT_N GPIOM0

# IRQ_UV_DETECT_N
gpio_export IRQ_UV_DETECT_N GPIOM1

# FM_HSC_TIMER_EXP_N
gpio_export FM_HSC_TIMER_EXP_N GPIOM2

# P3V3_STBY_MON_ALERT
gpio_export P3V3_STBY_MON_ALERT GPIOM3

# FM_TPM_BMC_PRES_N
gpio_export FM_TPM_BMC_PRES_N GPIOM4

# SPI_BMC_BT_WP0_N
gpio_export SPI_BMC_BT_WP0_N GPIOM5

# To enable GPION
#devmem_clear_bit $(scu_addr 88) 2
#devmem_clear_bit $(scu_addr 88) 3
#devmem_clear_bit $(scu_addr 88) 4
#devmem_clear_bit $(scu_addr 88) 5
#devmem_clear_bit $(scu_addr 88) 6
#devmem_clear_bit $(scu_addr 88) 7

# REMOTE_DEBUG_DIS_N
gpio_export REMOTE_DEBUG_DIS_N GPION2

# FM_SOL_UART_CH_SEL
gpio_export FM_SOL_UART_CH_SEL GPION3

# IRQ_DIMM_SAVE_LVT3_N
gpio_export IRQ_DIMM_SAVE_LVT3_N GPION4

# LED_8S_2S_MODE_N
gpio_export LED_8S_2S_MODE_N GPION5
gpio_set LED_8S_2S_MODE_N 1

# Set debounce timer #1 value to 0x12E1FC ~= 100ms
$DEVMEM 0x1e780050 32 0x12E1FC

# Select debounce timer #1 for GPION6
devmem_clear_bit 0x1e780100 14
devmem_set_bit 0x1e780104 14

# FM_POST_CARD_PRES_BMC_N
gpio_export FM_POST_CARD_PRES_BMC_N GPION6

# FM_PWRBRK_N
gpio_export FM_PWRBRK_N GPION7

# To enable GPIOO
#devmem_clear_bit $(scu_addr 88) 8
#devmem_clear_bit $(scu_addr 88) 9
#devmem_clear_bit $(scu_addr 88) 10
#devmem_clear_bit $(scu_addr 88) 11
#devmem_clear_bit $(scu_addr 88) 12
#devmem_clear_bit $(scu_addr 88) 13
#devmem_clear_bit $(scu_addr 88) 14
#devmem_clear_bit $(scu_addr 88) 15

# FM_BMC_SKT_ID_0
gpio_export FM_BMC_SKT_ID_0 GPIOO0

# FM_BMC_SKT_ID_1
gpio_export FM_BMC_SKT_ID_1 GPIOO1

# FM_BMC_SKT_ID_2
gpio_export FM_BMC_SKT_ID_2 GPIOO2

# FM_SMB_RETIMER_1_ISO_EN
gpio_export FM_SMB_RETIMER_1_ISO_EN GPIOO3

# FM_SMB_RETIMER_2_ISO_EN
gpio_export FM_SMB_RETIMER_2_ISO_EN GPIOO4

# PRSNT_UPI_BD_1_N
gpio_export PRSNT_UPI_BD_1_N GPIOO5

# PRSNT_UPI_BD_2_N
gpio_export PRSNT_UPI_BD_2_N GPIOO6

# PRSNT_UPI_BD_3_N
gpio_export PRSNT_UPI_BD_3_N GPIOO7

# To enable GPIOP
#devmem_clear_bit $(scu_addr 88) 16
#devmem_clear_bit $(scu_addr 88) 17
#devmem_clear_bit $(scu_addr 88) 18
#devmem_clear_bit $(scu_addr 88) 19
#devmem_clear_bit $(scu_addr 88) 20
#devmem_clear_bit $(scu_addr 88) 21
#devmem_clear_bit $(scu_addr 88) 22
#devmem_clear_bit $(scu_addr 88) 23

# PRSNT_UPI_BD_4_N
gpio_export PRSNT_UPI_BD_4_N GPIOP0

# SMB_BMC_ASICBOX_EN
gpio_export SMB_BMC_ASICBOX_EN GPIOP1

# FM_CM_READY_N
gpio_export FM_CM_READY_N GPIOP2

# RST_CM_N
gpio_export RST_CM_N GPIOP3
gpio_set RST_CM_N 1

# FM_ASIC_POWER_EN
gpio_export FM_ASIC_POWER_EN GPIOP4

# FM_ASIC_PWRGD
gpio_export FM_ASIC_PWRGD GPIOP5

# FM_BLADE_ID_0
gpio_export FM_BLADE_ID_0 GPIOP6

# FM_BLADE_ID_1
gpio_export FM_BLADE_ID_1 GPIOP7

# To enable GPIOQ
#devmem_clear_bit $(scu_addr 2c) 1
#devmem_clear_bit $(scu_addr 2c) 29

# RST_TCA9545_DDR_MUX_N
gpio_export RST_TCA9545_DDR_MUX_N GPIOQ6

# FM_BMC_CPU_PWR_DEBUG_N
gpio_export FM_BMC_CPU_PWR_DEBUG_N GPIOQ7

# To enable GPIOR
#devmem_clear_bit $(scu_addr 88) 25
#devmem_clear_bit $(scu_addr 88) 26
#devmem_clear_bit $(scu_addr 88) 27
#devmem_clear_bit $(scu_addr 88) 28
#devmem_clear_bit $(scu_addr 88) 29
#devmem_clear_bit $(scu_addr 88) 30
#devmem_clear_bit $(scu_addr 88) 31

# DBP_PRESENT_N
gpio_export DBP_PRESENT_N GPIOR1

# SMB_BMC_MM5_ALT_N
gpio_export SMB_BMC_MM5_ALT_N GPIOR2

# SMB_BMC_MM5_RST_N
gpio_export SMB_BMC_MM5_RST_N GPIOR3

# JTAG_MUX_SEL_0
gpio_export JTAG_MUX_SEL_0 GPIOR4

# JTAG_MUX_SEL_1
gpio_export JTAG_MUX_SEL_1 GPIOR5

# FM_BIOS_SPI_BMC_CTRL
gpio_export FM_BIOS_SPI_BMC_CTRL GPIOR6

# FM_SYS_THROTTLE_LVC3
gpio_export FM_SYS_THROTTLE_LVC3 GPIOR7

# To enable GPIOS
#devmem_clear_bit $(scu_addr 8C) 0
#devmem_clear_bit $(scu_addr 8C) 1
#devmem_clear_bit $(scu_addr 8C) 2
#devmem_clear_bit $(scu_addr 8C) 3
#devmem_clear_bit $(scu_addr 8C) 4
#devmem_clear_bit $(scu_addr 8C) 5
#devmem_clear_bit $(scu_addr 8C) 6
#devmem_clear_bit $(scu_addr 8C) 7

# FM_MODULAR_ASD_EN
gpio_export FM_MODULAR_ASD_EN GPIOS0

# DBP_SYSPWROK
gpio_export DBP_SYSPWROK GPIOS1

# RST_RSMRST_BMC_N
gpio_export RST_RSMRST_BMC_N GPIOS2

# IRQ_SML0_ALERT_MUX_N
gpio_export IRQ_SML0_ALERT_MUX_N GPIOS3

# FP_LOCATE_LED, ID LED
gpio_export FP_LOCATE_LED GPIOS4
gpio_set FP_LOCATE_LED 0

# FP_LED_STATUS_AMBER_N
gpio_export FP_LED_STATUS_AMBER_N GPIOS5

# FM_2S_FAN_IO_EXP_RST
gpio_export FM_2S_FAN_IO_EXP_RST GPIOS6

# RST_SMB_OCP_MUX_N
gpio_export RST_SMB_OCP_MUX_N GPIOS7

# To enable GPIOT
#devmem_clear_bit $(scu_addr 48) 29
#devmem_clear_bit $(scu_addr 48) 30
#devmem_set_bit $(scu_addr a0) 0
#devmem_set_bit $(scu_addr a0) 4
#devmem_set_bit $(scu_addr a0) 5
#devmem_set_bit $(scu_addr a0) 6

# FM_FORCE_BMC_UPDATE_N
gpio_export FM_FORCE_BMC_UPDATE_N GPIOT0

# FM_BMC_RBT_ISOLATE_N
gpio_export FM_BMC_RBT_ISOLATE_N GPIOT4

# FM_FAST_PROCHOT_EN_N
gpio_export FM_FAST_PROCHOT_EN_N GPIOT5

# OCP_V3_NIC_2_PWR_GOOD
gpio_export OCP_V3_NIC_2_PWR_GOOD GPIOT6

# To enable GPIOU
#devmem_set_bit $(scu_addr a0) 10
#devmem_set_bit $(scu_addr a0) 11
#devmem_set_bit $(scu_addr a0) 13

# BOARD_ID_MUX_SEL
gpio_export BOARD_ID_MUX_SEL GPIOU2
gpio_set BOARD_ID_MUX_SEL 0
mkdir -p /tmp/cache_store
echo $(($(gpio_get FM_BOARD_BMC_REV_ID2)<<2 |
        $(gpio_get FM_BOARD_BMC_REV_ID1)<<1 |
        $(gpio_get FM_BOARD_BMC_REV_ID0))) > /tmp/cache_store/mb_rev

echo $(($(gpio_get FM_BOARD_BMC_SKU_ID5)<<5 |
        $(gpio_get FM_BOARD_BMC_SKU_ID4)<<4 |
        $(gpio_get FM_BOARD_BMC_SKU_ID3)<<3 |
        $(gpio_get FM_BOARD_BMC_SKU_ID2)<<2 |
        $(gpio_get FM_BOARD_BMC_SKU_ID1)<<1 |
        $(gpio_get FM_BOARD_BMC_SKU_ID0))) > /tmp/cache_store/mb_sku

gpio_set BOARD_ID_MUX_SEL 1

# BMC_FORCE_NM_THROTTLE_N
gpio_export BMC_FORCE_NM_THROTTLE_N GPIOU3

# FM_CPU0_MEMHOT_OUT_N
gpio_export FM_CPU0_MEMHOT_OUT_N GPIOU5

# To enable GPIOV
#devmem_set_bit $(scu_addr a0) 19

# OCP_V3_NIC_1_PWR_GOOD
gpio_export OCP_V3_NIC_1_PWR_GOOD GPIOV3

# To enable GPIOY
#devmem_clear_scu70_bit 19
#devmem_clear_bit $(scu_addr 94) 10
#devmem_clear_bit $(scu_addr 94) 11
#devmem_clear_bit $(scu_addr a4) 8
#devmem_clear_bit $(scu_addr a4) 9
#devmem_clear_bit $(scu_addr a4) 10
#devmem_clear_bit $(scu_addr a4) 11

# FM_SLPS3_N
gpio_export FM_SLPS3_N GPIOY0

# FM_SLPS4_N
gpio_export FM_SLPS4_N GPIOY1

# PWRGD_SYS_PWROK, power GOOD from CPLD
gpio_export PWRGD_SYS_PWROK GPIOY2

# FM_BMC_ONCTL_N
gpio_export FM_BMC_ONCTL_N GPIOY3

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

# FM_CPU_CATERR_LVT3_N
gpio_export FM_CPU_CATERR_LVT3_N GPIOZ0

# PWRGD_CPU0_LVC3
gpio_export PWRGD_CPU0_LVC3 GPIOZ1

# FM_CPU_MSMI_LVT3_N
gpio_export FM_CPU_MSMI_LVT3_N GPIOZ2

# FM_BMC_PCH_SCI_LPC_N
gpio_export FM_BMC_PCH_SCI_LPC_N GPIOZ3

# UV_HIGH_SET
gpio_export UV_HIGH_SET GPIOZ4

# FM_OC_DETECT_EN_N
gpio_export FM_OC_DETECT_EN_N GPIOZ5

# FM_SML1_PMBUS_ALERT_EN_N
gpio_export FM_SML1_PMBUS_ALERT_EN_N GPIOZ6

# FM_BMC_LED_CATERR_N
gpio_export FM_BMC_LED_CATERR_N GPIOZ7
gpio_set FM_BMC_LED_CATERR_N 1

# To enable GPIOAA
#devmem_clear_bit $(scu_addr a4) 24
#devmem_clear_bit $(scu_addr a4) 25
#devmem_clear_bit $(scu_addr a4) 26
#devmem_clear_bit $(scu_addr a4) 27
#devmem_clear_bit $(scu_addr a4) 28
#devmem_clear_bit $(scu_addr a4) 29
#devmem_clear_bit $(scu_addr a4) 30
#devmem_clear_bit $(scu_addr a4) 31

# P3V_BAT_SCALED_EN
gpio_export P3V_BAT_SCALED_EN GPIOAA0
gpio_set P3V_BAT_SCALED_EN 0

# IRQ_SML1_PMBUS_BMC_ALERT_N
gpio_export IRQ_SML1_PMBUS_BMC_ALERT_N GPIOAA1

# FM_PVCCIN_CPU0_PWR_IN_ALERT_N
gpio_export FM_PVCCIN_CPU0_PWR_IN_ALERT_N GPIOAA2

# FM_PVCCIN_CPU1_PWR_IN_ALERT_N
gpio_export FM_PVCCIN_CPU1_PWR_IN_ALERT_N GPIOAA3

# FM_BMC_PREQ_N
gpio_export FM_BMC_PREQ_N GPIOAA4

# FM_JTAG_TCK_MUX_SEL
gpio_export FM_JTAG_TCK_MUX_SEL GPIOAA5

# IRQ_SMI_ACTIVE_BMC_N
gpio_export IRQ_SMI_ACTIVE_BMC_N GPIOAA6

# FM_BIOS_POST_CMPLT_BMC_N
gpio_export FM_BIOS_POST_CMPLT_BMC_N GPIOAA7

# To enable GPIOAB
#devmem_clear_bit $(scu_addr a8) 0
#devmem_clear_bit $(scu_addr a8) 1
#devmem_clear_bit $(scu_addr a8) 3

# FM_BMC_BMCINIT
gpio_export FM_BMC_BMCINIT GPIOAB0

# PECI_MUX_SELECT
gpio_export PECI_MUX_SELECT GPIOAB1
gpio_set PECI_MUX_SELECT 1

# PWRGD_BMC_PS_PWROK
gpio_export PWRGD_BMC_PS_PWROK GPIOAB3


# I/O Expander TCA9539 0xEC
gpio_export_ioexp 4-0076 FM_SLOT1_PRSNT_N 0
gpio_export_ioexp 4-0076 FM_SLOT2_PRSNT_N 1
gpio_export_ioexp 4-0076 RST_RTCRST_N 10
gpio_export_ioexp 4-0076 RST_USB_HUB_N 11

# I/O Expander TCA9539 0xEE
gpio_export_ioexp 4-0077 IRQ_PVCCIN_CPU0_VRHOT_LVC3_N 0
gpio_export_ioexp 4-0077 IRQ_PVCCIN_CPU1_VRHOT_LVC3_N 1
gpio_export_ioexp 4-0077 IRQ_PVDDQ_ABC_CPU0_VRHOT_LVC3_N 2
gpio_export_ioexp 4-0077 IRQ_PVDDQ_DEF_CPU0_VRHOT_LVC3_N 3
gpio_export_ioexp 4-0077 IRQ_PVDDQ_ABC_CPU1_VRHOT_LVC3_N 4
gpio_export_ioexp 4-0077 IRQ_PVDDQ_DEF_CPU1_VRHOT_LVC3_N 5
gpio_export_ioexp 4-0077 FM_CPU0_SKTOCC_LVT3_PLD_N 6
gpio_export_ioexp 4-0077 FM_CPU1_SKTOCC_LVT3_PLD_N 7
gpio_export_ioexp 4-0077 HP_LVC3_OCP_V3_2_PRSNT2_N 8
gpio_export_ioexp 4-0077 HP_LVC3_OCP_V3_1_PRSNT2_N 9
