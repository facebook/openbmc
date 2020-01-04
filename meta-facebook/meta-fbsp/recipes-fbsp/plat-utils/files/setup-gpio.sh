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

# FM_BOARD_BMC_REV_ID0
gpio_export FM_BOARD_BMC_REV_ID0 GPIOA0

# FM_BOARD_BMC_REV_ID1 / FM_CPU0_THERMTRIP_LVT3_PLD_N
gpio_export FM_CPU0_THERMTRIP_LVT3_PLD_N GPIOA1
ln -n /tmp/gpionames/FM_CPU0_THERMTRIP_LVT3_PLD_N /tmp/gpionames/FM_BOARD_BMC_REV_ID1

# FM_SPD_DDRCPU_LVLSHFT_EN
gpio_export FM_SPD_DDRCPU_LVLSHFT_EN GPIOA3


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


# FM_BOARD_BMC_REV_ID2 / FM_CPU1_THERMTRIP_LVT3_PLD_N
gpio_export FM_CPU1_THERMTRIP_LVT3_PLD_N GPIOD0
ln -n /tmp/gpionames/FM_CPU1_THERMTRIP_LVT3_PLD_N /tmp/gpionames/FM_BOARD_BMC_REV_ID2

# IRQ_TPM_BMC_SPI_N
gpio_export IRQ_TPM_BMC_SPI_N GPIOD1

# OCP_V3_1_NIC_PWR_GOOD
gpio_export OCP_V3_1_NIC_PWR_GOOD GPIOD6

# OCP_V3_2_NIC_PWR_GOOD
gpio_export OCP_V3_2_NIC_PWR_GOOD GPIOD7


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

# ALT_PDB_N
gpio_export ALT_PDB_N GPIOE7


# FM_CPU_ERR0_LVT3_N
gpio_export FM_CPU_ERR0_LVT3_N GPIOF0

# FM_CPU_ERR1_LVT3_N
gpio_export FM_CPU_ERR1_LVT3_N GPIOF1

# FM_CPU_ERR2_LVT3_N
gpio_export FM_CPU_ERR2_LVT3_N GPIOF2

# FM_BMC_DBP_PRESENT_N
gpio_export FM_BMC_DBP_PRESENT_N GPIOF3

# FM_BMC_UV_ADR_TRIGGER_EN
gpio_export FM_BMC_UV_ADR_TRIGGER_EN GPIOF4

# FM_BMC_FORCE_ADR_N
gpio_export FM_BMC_FORCE_ADR_N GPIOF5

# RST_PLTRST_BMC_N
gpio_export RST_PLTRST_BMC_N GPIOF6

# JTAG_BMC_PRDY_N
gpio_export JTAG_BMC_PRDY_N GPIOF7


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

# SLIVER_CONNA_BMC_ALERT_N
gpio_export SLIVER_CONNA_BMC_ALERT_N GPIOG7


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


# PRSNT_2C_PCIE_SLIVER_CABLE_1_N
gpio_export PRSNT_2C_PCIE_SLIVER_CABLE_1_N GPIOI0

# PRSNT_2C_PCIE_SLIVER_CABLE_2_N
gpio_export PRSNT_2C_PCIE_SLIVER_CABLE_2_N GPIOI1

# SLIVER_CONNB_BMC_ALERT_N
gpio_export SLIVER_CONNB_BMC_ALERT_N GPIOI2

# FM_PFR_ACTIVE_N
gpio_export FM_PFR_ACTIVE_N GPIOI3


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

# BMC_FORCE_NM_THROTTLE_N
gpio_export BMC_FORCE_NM_THROTTLE_N GPIOL5


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


# BOARD_ID_MUX_SEL
gpio_export BOARD_ID_MUX_SEL GPION2
gpio_set BOARD_ID_MUX_SEL 0
echo $(($(gpio_get FM_BOARD_BMC_REV_ID2)<<2 |
        $(gpio_get FM_BOARD_BMC_REV_ID1)<<1 |
        $(gpio_get FM_BOARD_BMC_REV_ID0))) > /tmp/mb_rev

echo $(($(gpio_get FM_BOARD_BMC_SKU_ID5)<<5 |
        $(gpio_get FM_BOARD_BMC_SKU_ID4)<<4 |
        $(gpio_get FM_BOARD_BMC_SKU_ID3)<<3 |
        $(gpio_get FM_BOARD_BMC_SKU_ID2)<<2 |
        $(gpio_get FM_BOARD_BMC_SKU_ID1)<<1 |
        $(gpio_get FM_BOARD_BMC_SKU_ID0))) > /tmp/mb_sku

gpio_set BOARD_ID_MUX_SEL 1

# FM_SOL_UART_CH_SEL
gpio_export FM_SOL_UART_CH_SEL GPION3

# IRQ_DIMM_SAVE_LVT3_N
gpio_export IRQ_DIMM_SAVE_LVT3_N GPION4

# FM_REMOTE_DEBUG_EN_DET
gpio_export FM_REMOTE_DEBUG_EN_DET GPION5

# Set debounce timer #1 value to 0x12E1FC ~= 100ms
$DEVMEM 0x1e780050 32 0x12E1FC

# Select debounce timer #1 for GPION6
devmem_clear_bit 0x1e780100 14
devmem_set_bit 0x1e780104 14

# FM_POST_CARD_PRES_BMC_N
gpio_export FM_POST_CARD_PRES_BMC_N GPION6

# FM_PWRBRK_N
gpio_export FM_PWRBRK_N GPION7


# RST_TCA9545_DDR_MUX_N
gpio_export RST_TCA9545_DDR_MUX_N GPIOQ6

# FM_BMC_CPU_PWR_DEBUG_N
gpio_export FM_BMC_CPU_PWR_DEBUG_N GPIOQ7


# DBP_PRESENT_N
gpio_export DBP_PRESENT_N GPIOR1

# SMB_BMC_MM5_ALT_N
gpio_export SMB_BMC_MM5_ALT_N GPIOR2

# SMB_BMC_MM5_RST_N
gpio_export SMB_BMC_MM5_RST_N GPIOR3

# SMB_SLOT1_ALT_N
gpio_export SMB_SLOT1_ALT_N GPIOR4

# SMB_SLOT2_ALT_N
gpio_export SMB_SLOT2_ALT_N GPIOR5

# FM_BIOS_SPI_BMC_CTRL
gpio_export FM_BIOS_SPI_BMC_CTRL GPIOR6

# FM_SYS_THROTTLE_LVC3
gpio_export FM_SYS_THROTTLE_LVC3 GPIOR7


# FM_BMC_REMOTE_DEBUG_EN
gpio_export FM_BMC_REMOTE_DEBUG_EN GPIOS0

# DBP_SYSPWROK
gpio_export DBP_SYSPWROK GPIOS1

# RST_RSMRST_BMC_N
gpio_export RST_RSMRST_BMC_N GPIOS2

# IRQ_SML0_ALERT_MUX_N
gpio_export IRQ_SML0_ALERT_MUX_N GPIOS3

# FP_LOCATE_LED, ID LED
gpio_export FP_LOCATE_LED GPIOS4
gpio_set FP_LOCATE_LED 0

# FM_MRC_DEBUG_EN
gpio_export FM_MRC_DEBUG_EN GPIOS5

# FM_2S_FAN_IO_EXP_RST
gpio_export FM_2S_FAN_IO_EXP_RST GPIOS6

# RST_SMB_OCP_MUX_N
gpio_export RST_SMB_OCP_MUX_N GPIOS7


# FM_FORCE_BMC_UPDATE_N
gpio_export FM_FORCE_BMC_UPDATE_N GPIOT0

# FM_FAST_PROCHOT_EN_N
gpio_export FM_FAST_PROCHOT_EN_N GPIOT5


# FM_CPU0_MEMHOT_OUT_N
gpio_export FM_CPU0_MEMHOT_OUT_N GPIOU5


# FM_SLPS3_N
gpio_export FM_SLPS3_N GPIOY0

# FM_SLPS4_N
gpio_export FM_SLPS4_N GPIOY1

# PWRGD_SYS_PWROK, power GOOD from CPLD
gpio_export PWRGD_SYS_PWROK GPIOY2

# FM_BMC_ONCTL_N
gpio_export FM_BMC_ONCTL_N GPIOY3


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


# P3V_BAT_SCALED_EN
gpio_export P3V_BAT_SCALED_EN GPIOAA0
gpio_set P3V_BAT_SCALED_EN 0

# IRQ_SML1_PMBUS_BMC_ALERT_N
gpio_export IRQ_SML1_PMBUS_BMC_ALERT_N GPIOAA1

# FM_PVCCIN_CPU0_PWR_IN_ALERT_N
gpio_export FM_PVCCIN_CPU0_PWR_IN_ALERT_N GPIOAA2

# FM_PVCCIN_CPU1_PWR_IN_ALERT_N
gpio_export FM_PVCCIN_CPU1_PWR_IN_ALERT_N GPIOAA3

# JTAG_DBP_CPU_PREQ_N
gpio_export JTAG_DBP_CPU_PREQ_N GPIOAA4

# FM_JTAG_TCK_MUX_SEL
gpio_export FM_JTAG_TCK_MUX_SEL GPIOAA5

# IRQ_SMI_ACTIVE_BMC_N
gpio_export IRQ_SMI_ACTIVE_BMC_N GPIOAA6

# FM_BIOS_POST_CMPLT_BMC_N
gpio_export FM_BIOS_POST_CMPLT_BMC_N GPIOAA7


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
gpio_export_ioexp 4-0076 BMC_PWM_EN 7
gpio_set BMC_PWM_EN 1

# I/O Expander TCA9539 0xEE
gpio_export_ioexp 4-0077 HP_LVC3_OCP_V3_2_PRSNT2_N 8
gpio_export_ioexp 4-0077 HP_LVC3_OCP_V3_1_PRSNT2_N 9

# I/O Expander PCA9552PW 0xC0
for i in {0..3};
do
  gpio_export_ioexp 8-0060 FM_FAN${i}_POWER_LED_N ${i}
  gpio_export_ioexp 8-0060 FM_FAN${i}_FAULT_LED_N $((i+4))
  gpio_export_ioexp 8-0060 FM_FAN${i}_BUF_PRESENT_R $((i+8))
  gpio_export_ioexp 8-0060 FM_FAN${i}_P12V_EN  $((i+12))
done

# I/O Expander PCA9552PW 0xC2
for i in {4..7};
do
  gpio_export_ioexp 8-0061 FM_FAN${i}_POWER_LED_N $((i-4))
  gpio_export_ioexp 8-0061 FM_FAN${i}_FAULT_LED_N $((i))
  gpio_export_ioexp 8-0061 FM_FAN${i}_BUF_PRESENT_R $((i+4))
  gpio_export_ioexp 8-0061 FM_FAN${i}_P12V_EN $((i+8))
done

# Disable JTAG engine
devmem 0x1e6e4008 32 0x0
