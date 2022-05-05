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

# ==GPIOA==


# ==GPIOB==
# FM_ME_BT_DONE
#gpio_export FM_ME_BT_DONE GPIOB0

# SMB_BMC_ALERT3_N
gpio_export SMB_BMC_ALERT3_N GPIOB2

# SMB_BMC_ALERT4_N
gpio_export SMB_BMC_ALERT4_N GPIOB3

# FM_BMC_MUX_CS_SPI_SEL_0
gpio_export FM_BMC_MUX_CS_SPI_SEL_0 GPIOB4
gpio_set FM_BMC_MUX_CS_SPI_SEL_0 0

# FM_ID_LED_N
gpio_export FM_ID_LED_N GPIOB5
gpio_set FM_ID_LED_N 0

# FM_ESPI_PLD_EN
gpio_export FM_ESPI_PLD_EN GPIOB6

# IRQ_BMC_CPU_NMI
#gpio_export IRQ_BMC_CPU_NMI GPIOB7

# ==GPIOC==
# RST_PLTRST_R_N
gpio_export RST_PLTRST_R_N GPIOC0

#FM_CPU_RMCA_CATERR_LVT3_R_N
gpio_export FM_CPU_RMCA_CATERR_LVT3_R_N GPIOC1

#RST_SGPIO_RESET_BMC_N
gpio_export RST_SGPIO_RESET_BMC_N GPIOC3
gpio_set RST_SGPIO_RESET_BMC_N 1

#RST_ROT_CPU_R1_N
gpio_export RST_ROT_CPU_R1_N GPIOC4

#H_CPU_CATERR_LVC2_R2_N
gpio_export H_CPU_CATERR_LVC2_R2_N GPIOC5

#FM_JTAG_BMC_MUX_SEL
gpio_export FM_JTAG_BMC_MUX_SEL GPIOC6
gpio_set FM_JTAG_BMC_MUX_SEL 1;

#IRQ_SGPIO_BMC_N
gpio_export IRQ_SGPIO_BMC_N GPIOC7

# ==GPIOD==
gpio_export GPU_FPGA_RST_N GPIOD0
gpio_set GPU_FPGA_RST_N 0

gpio_export CLK_BUF1_GPU_FPGA_OE_N GPIOD1
gpio_set CLK_BUF1_GPU_FPGA_OE_N 0

# ==GPIOE==
#FM_FLASH_LATCH_N_R1
gpio_export FM_FLASH_LATCH_N_R1 GPIOE0

#FLASH_WP_STATUS_R1
gpio_export FLASH_WP_STATUS_R1 GPIOE1

#FM_PLT_BMC_THERMTRIP_N_R
gpio_export FM_PLT_BMC_THERMTRIP_N_R GPIOE3

# ==GPIOF==

# ==GPIOG==

# SMB_BMC_ALERT15_N 
gpio_export SMB_BMC_ALERT15_N GPIOG6

# FM_BMC_DBP_PRSNT_N
gpio_export FM_BMC_DBP_PRSNT_N GPIOG7

# ==GPIOH==

# ==GPIOI==
# PD_BIOS_MUX_EN_N
gpio_export PD_BIOS_MUX_EN_N GPIOI5
gpio_set PD_BIOS_MUX_EN_N 0

# FM_THROTTLE_N
gpio_export FM_THROTTLE_N GPIOI6

# IRQ_PCH_SCI_WHEA_N
gpio_export IRQ_PCH_SCI_WHEA_N GPIOI7

# ==GPIOJ==


# ==GPIOK==


# ==GPIOL==
# FM_DBP_CPU_PREQ_GF_N
gpio_export FM_DBP_CPU_PREQ_GF_N GPIOL4
gpio_set FM_DBP_CPU_PREQ_GF_N 1

# FM_JTAG_TCK_MUX_BMC_SEL
#gpio_export FM_JTAG_TCK_MUX_BMC_SEL GPIOL5
#gpio_set FM_JTAG_TCK_MUX_BMC_SEL 0


# ==GPIOM==
# H_CPU0_PROCHOT_N
gpio_export H_CPU0_PROCHOT_N GPIOM0

# FM_VIRTUAL_RESEAT_BMC
gpio_export FM_VIRTUAL_RESEAT_BMC GPIOM1

# PWRGD_PSU_AC_PWROK_BMC
gpio_export PWRGD_PSU_AC_PWROK_BMC GPIOM2

# FM_PECI_SEL_N
gpio_export FM_PECI_SEL_N GPIOM3
gpio_set FM_PECI_SEL_N 1

# PWRGD_SYS_PWROK_BMC
gpio_export PWRGD_SYS_PWROK_BMC GPIOM4

# FM_DBP_BMC_PRDY_N
gpio_export FM_DBP_BMC_PRDY_N GPIOM5


# ==GPION==
# LED_POSTCODE_0
gpio_export LED_POSTCODE_0 GPION0

# LED_POSTCODE_1
gpio_export LED_POSTCODE_1 GPION1

# LED_POSTCODE_2
gpio_export LED_POSTCODE_2 GPION2

# LED_POSTCODE_3
gpio_export LED_POSTCODE_3 GPION3

# LED_POSTCODE_4
gpio_export LED_POSTCODE_4 GPION4

# LED_POSTCODE_5
gpio_export LED_POSTCODE_5 GPION5

# LED_POSTCODE_6
gpio_export LED_POSTCODE_6 GPION6

# LED_POSTCODE_7
gpio_export LED_POSTCODE_7 GPION7


# ==GPIOO==
# FM_PMBUS_ALERT_EN
gpio_export FM_PMBUS_ALERT_EN GPIOO0

# FM_PCEI_M2_SSD0_PRSNT_N
gpio_export FM_PCEI_M2_SSD0_PRSNT_N GPIOO2

# FM_JTAG_BMC_MUX_SEL
#gpio_export FM_JTAG_BMC_MUX_SEL GPIOO3

# FM_TPM_PRSNT_0_N
gpio_export FM_TPM_PRSNT_0_N GPIOO5

# FM_ADR_TRIGGER_N
#gpio_export FM_ADR_TRIGGER_N GPIOO4

# FM_ADR_COMPLETE
#gpio_export FM_ADR_COMPLETE GPIOO6

# FM_SPD_REMOTE_EN
gpio_export FM_SPD_REMOTE_EN GPIOO7


# ==GPIOP==
# SYS_BMC_PWRBTN_IN
gpio_export SYS_BMC_PWRBTN_IN GPIOP0

# SYS_BMC_PWRBTN_OUT
gpio_export SYS_BMC_PWRBTN_OUT GPIOP1
gpio_set SYS_BMC_PWRBTN_OUT 1

# SYS_BMC_RSTBTN_IN
gpio_export ID_RST_BTN_BMC_IN GPIOP2

# SYS_BMC_RSTBTN_OUT
gpio_export ID_RST_BTN_BMC_OUT GPIOP3
gpio_set ID_RST_BTN_BMC_OUT 1

# BMC_PWR_LED
gpio_export BMC_PWR_LED GPIOP4
gpio_set BMC_PWR_LED 0

# H_CPU0_MEMTRIP_LVC1_N
#gpio_export H_CPU0_MEMTRIP_LVC1_N GPIOP5

# H_CPU1_MEMTRIP_LVC1_N
#gpio_export H_CPU1_MEMTRIP_LVC1_N GPIOP6


# ==GPIOQ==
# IRQ_PCH_TPM_SPI_N
gpio_export IRQ_PCH_TPM_SPI_N GPIOQ0

# USB_OC0_REAR_R_N
gpio_export USB_OC0_REAR_R_N GPIOQ1

# GPU_NVS_PWR_BRAKE_R_N
gpio_export GPU_NVS_PWR_BRAKE_R_N GPIOQ2
gpio_set GPU_NVS_PWR_BRAKE_R_N 1

# GPU_WP_HW_CTRL_R_N
gpio_export GPU_WP_HW_CTRL_R_N GPIOQ3
gpio_set GPU_WP_HW_CTRL_R_N 1

# FM_HDD_LED_N
gpio_export FM_HDD_LED_N GPIOQ4
gpio_set FM_HDD_LED_N 0

# FM_P12V_HSC_ENABLE_R1
gpio_export FM_P12V_HSC_ENABLE_R1 GPIOQ5
gpio_set FM_P12V_HSC_ENABLE_R1 1

# FM_PCH_BEEP_LED
gpio_export FM_PCH_BEEP_LED GPIOQ6
gpio_set FM_PCH_BEEP_LED 0

# FM_SOL_UART_CH_SEL
gpio_export FM_SOL_UART_CH_SEL GPIOQ7


# ==GPIOR==
# IRQ_OC_DETECT_N 
gpio_export IRQ_OC_DETECT_N GPIOR0

# IRQ_UV_DETECT_N
gpio_export IRQ_UV_DETECT_N GPIOR1

# IRQ_OC_DETECT_EN_N
gpio_export IRQ_OC_DETECT_EN_N GPIOR2

# IRQ_CPU0_VRHOT_N 
#gpio_export IRQ_CPU0_VRHOT_N GPIOR3

# IRQ_CPU1_VRHOT_N
#gpio_export IRQ_CPU1_VRHOT_N GPIOR4

# PWRGD_PCH_PWROK 
gpio_export PWRGD_PCH_PWROK GPIOR5

# PWRGD_CPUPWRGD_LVC1
gpio_export PWRGD_CPUPWRGD_LVC1 GPIOR6

# FM_BIOS_POST_CMPLT_BMC_N 
#gpio_export FM_BIOS_POST_CMPLT_BMC_N GPIOR7


# ==GPIOS==
# IRQ_BMC_PCH_NMI_N
#gpio_export IRQ_BMC_PCH_NMI_N GPIOS0

# BMC_MONITER
#gpio_export BMC_MONITER GPIOS1

# FM_THERMTRIP_N
gpio_export FM_THERMTRIP_N GPIOS2

# FM_TPM_PRSNT_1_N
gpio_export FM_TPM_PRSNT_1_N GPIOS3

# FM_BMC_DEBUG_SW_N
gpio_export FM_BMC_DEBUG_SW_N GPIOS4

# H_CPU1_PROCHOT_N
gpio_export H_CPU1_PROCHOT_N GPIOS5


# ==GPIU==
# SMB_BMC_ALERT9_N
gpio_export SMB_BMC_ALERT9_N GPIOU0

# SMB_BMC_ALERT10_N
gpio_export SMB_BMC_ALERT10_N GPIOU1

# SMB_BMC_ALERT12_N
gpio_export SMB_BMC_ALERT12_N GPIOU3

# BSM_PRSNT_R_N
gpio_export BSM_PRSNT_R_N GPIOU4

# FM_BMC_EN_DET
gpio_export FM_BMC_EN_DET GPIOU5

# FM_PCHHOT_N
gpio_export FM_PCHHOT_N GPIOU6

# SMB_BMC_ALERT16_N
gpio_export SMB_BMC_ALERT16_N GPIOU7

# ==GPIOV==
# FM_SLPS3_GF_N
gpio_export FM_SLPS3_GF_N GPIOV0

# RST_RSMRST_R_N
gpio_export RST_RSMRST_R_N GPIOV1

gpio_export BMC_ID_BEEP_SEL_R1 GPIOV2
#gpio_set BMC_ID_BEEP_SEL_R1 1

# BATTERY_DETECT
gpio_export BATTERY_DETECT GPIOV4
gpio_set BATTERY_DETECT 0

# IRQ_TPM_SPI_N
gpio_export IRQ_TPM_SPI_N GPIOV7

# ==GPIX==
# PWRGD_DSW_PWROK_R1
#gpio_export PWRGD_DSW_PWROK_R1 GPIOX1

# IRQ_SMI_ACTIVE_BMC_N
gpio_export IRQ_SMI_ACTIVE_BMC_N GPIOX2

# ==GPIOY==
gpio_export RST_WDTRST_PLD_R1_N GPIOY1

#FM_BIOS_DEBUG_EN_R_N
#gpio_export FM_BIOS_DEBUG_EN_R_N GPIOY2

#FM BMC_EMMC_RST_R_N
gpio_export BMC_EMMC_RST_R_N GPIOY3

# ==GPIOZ==
# BMC_READY_N
#gpio_export BMC_READY_N GPIOZ0
#gpio_set BMC_READY_N 0

#FM_DEBUG_PORT_PRSNT_R1_N
gpio_export FM_DEBUG_PORT_PRSNT_R1_N GPIOZ6


# ==GPIO18D==
# EMMC Func GPIO18D0 - GPIO18D7

# ==SGPIO==
sgpio_export FM_OCP_SFF_PWR_GOOD_R 0
sgpio_export IRQ_SML1_PMBUS_PLD_ALERT_N 2
sgpio_export PWRGD_PVNN_MAIN_CPU0 4
sgpio_export PWRGD_PVPP_HBM_CPU1 6
sgpio_export GPU_FPGA_READY_ISO_R 8
sgpio_export FM_ADR_ACK_R 10
sgpio_export IRQ_PVCCD_CPU0_VRHOT_LVC3_N 12
sgpio_export FM_DIS_PS_PWROK_DLY 14
sgpio_export IRQ_PSYS_CRIT_N 16
sgpio_export IRQ_PVCCD_CPU1_VRHOT_LVC3_N 18
sgpio_export FM_SYS_THROTTLE_R_N 20
sgpio_export FM_PCH_PLD_GLB_RST_WARN_N 22
sgpio_export HPDB_HSC_PWRGD_ISO_R 24
sgpio_export FM_UV_ADR_TRIGGER_R_N 26
sgpio_export FM_PCH_BMC_THERMTRIP_N 28
sgpio_export FM_PCHHOT_R_N 30
sgpio_export FM_SWB_BIC_READY_ISO_R_N 32
sgpio_export FM_PCH_PRSNT_N 34
sgpio_export IRQ_HSC_FAULT_R1_N 36
sgpio_export FM_OCP_V3_2_PWR_GOOD_R 38
sgpio_export GPU_FPGA_THERM_OVERT_ISO_R_N 40
sgpio_export GPU_FPGA_OVERT_ISO_R_N 42
sgpio_export PWRGD_PVNN_PCH_AUX 44
sgpio_export BIC_MONITER_ISO_R 46
sgpio_export PWRGD_PVNN_MAIN_CPU1 48
sgpio_export PWRGD_PVPP_HBM_CPU0 50
sgpio_export GPU_HMC_PRSNT_ISO_R_N 52
sgpio_export GPU_PRSNT_N_ISO_R 54
sgpio_export PWRGD_P3V3 56
sgpio_export PWRGD_PVCCFA_EHV_FIVRA_CPU0 58
sgpio_export FM_GPU_PWRGD_ISO_R 60
sgpio_export RST_BMC_FROM_SWB_ISO_R_N 62
sgpio_export GPU_BASE_HMC_READY_ISO_R 64
sgpio_export PWRGD_PVCCD_HV_CPU0 66
sgpio_export PWRGD_PVCCIN_CPU0 68
sgpio_export PWRGD_P3V3_AUX_PLD 70
sgpio_export PWRGD_PVCCINFAON_CPU0 72
sgpio_export PWRGD_PVCCIN_CPU1 74
sgpio_export PWRGD_PS_PWROK_PLD_R 76
sgpio_export PWRGD_PVCCD_HV_CPU1 78
sgpio_export PWRGD_PVCCFA_EHV_FIVRA_CPU1 80
sgpio_export PWRGD_PVCCINFAON_CPU1 82
sgpio_export PWRGD_PVCCFA_EHV_CPU1 84
sgpio_export PWRGD_PVCCFA_EHV_CPU0 86
sgpio_export PWRGD_P1V05_PCH_AUX 88
sgpio_export PWRGD_P1V8_PCH_AUX_PLD 90
sgpio_export PWRGD_SYS_PWROK_R 92
sgpio_export PWRGD_PCH_PWROK_R 94
sgpio_export E1S_0_P3V3_ADC_ALERT 96
sgpio_export P12V_SCM_ADC_ALERT 98
sgpio_export M2_0_P3V3_ADC_ALERT 100
sgpio_export OCP_V3_2_P3V3_ADC_ALERT 102
sgpio_export OCP_SFF_P3V3_ADC_ALERT 104
sgpio_export FM_UARTSW_LSB_N 106
sgpio_export FM_UARTSW_MSB_N 108
sgpio_export FM_THERMAL_ALERT_R_N 110
sgpio_export FM_CPU0_SKTOCC_LVT3_PLD_N 112
sgpio_export FM_CPU1_SKTOCC_LVT3_PLD_N 114
sgpio_export PWRGD_CPUPWRGD_LVC2_R1 116
sgpio_export H_CPU1_THERMTRIP_LVC1_N 118
sgpio_export IRQ_CPU1_VRHOT_N 120
sgpio_export H_CPU0_MEMTRIP_LVC1_N 122
sgpio_export H_CPU1_MEMHOT_OUT_LVC1_N 124
sgpio_export PWRGD_FAIL_CPU1_CD_R 126
sgpio_export PWRGD_FAIL_CPU1_GH_R 128
sgpio_export PWRGD_FAIL_CPU0_EF_R 130
sgpio_export PWRGD_FAIL_CPU0_AB_R 132
sgpio_export PWRGD_FAIL_CPU0_CD_R 134
sgpio_export H_CPU0_THERMTRIP_LVC1_N 136
sgpio_export H_CPU0_MEMHOT_OUT_LVC1_N 138
sgpio_export H_CPU_ERR2_LVC2_N 140
sgpio_export H_CPU_ERR1_LVC2_N 142
sgpio_export H_CPU_ERR0_LVC2_N 144
sgpio_export FM_BIOS_POST_CMPLT_BMC_N 146
sgpio_export FM_ME_BT_DONE 148
sgpio_export PWRGD_FAIL_CPU1_AB_R 150
sgpio_export PWRGD_FAIL_CPU1_EF_R 152
sgpio_export IRQ_CPU0_VRHOT_N 154
sgpio_export PWRGD_DSW_PWROK_R3 156
sgpio_export H_CPU1_MEMTRIP_LVC1_N 158
sgpio_export FM_ADR_COMPLETE 160
sgpio_export FM_BMC_DBP_PRESENT_R_N 162
sgpio_export FM_SLPS3_PLD_N 164
sgpio_export FM_SLPS4_PLD_N 166
sgpio_export FM_ADR_TRIGGER_N 168
sgpio_export FM_REMOTE_DEBUG_DET 170
sgpio_export E1S_0_PRSNT_N 172
sgpio_export IRQ_PCH_CPU_NMI_EVENT_N 174
sgpio_export FM_PS_EN_PLD_R 176
sgpio_export HSC_D_OC_R1 178
sgpio_export PWRGD_FAIL_CPU0_GH_R 180
sgpio_export H_CPU_RMCA_LVC2_R2_N 182
#sgpio_export H_CPU_CATERR_LVC2_R2_N 184
sgpio_export H_CPU1_PROCHOT_LVC1_R_N 186
sgpio_export IRQ_UV_DETECT_R_N 188
sgpio_export PCIE_M2_SSD0_PRSNT_N 190
sgpio_export FM_ME_AUTHN_FAIL 192
sgpio_export PRSNT_SWB_R_N 194
sgpio_export IRQ_PSU_ALERT_R_N 196
sgpio_export FM_THROTTLE_R_N 198
sgpio_export RST_PLTRST_PLD_N 200
sgpio_export H_CPU_PROCHOT_LVC1_N 202
sgpio_export FM_PCH_BEEP_LED 204
sgpio_export FM_HDD_LED_N 206
sgpio_export FM_PFR_NO_SERVICE_ACT_N 208
sgpio_export FM_PFR_UPDATE_N 210
sgpio_export FM_BOARD_BMC_SKU_ID4 212
sgpio_export FM_BOARD_BMC_SKU_ID3 214
sgpio_export FM_BOARD_BMC_SKU_ID2 216
sgpio_export FM_BOARD_BMC_SKU_ID1 218
sgpio_export FM_BOARD_BMC_SKU_ID0 220
sgpio_export FAB_BMC_REV_ID2 222
sgpio_export FAB_BMC_REV_ID1 224
sgpio_export FAB_BMC_REV_ID0 226




# == SGPIO OUT ==
sgpio_export FM_ESPI_PLD_R1_EN 1
gpio_set FM_ESPI_PLD_R1_EN 1

sgpio_export IRQ_BMC_CPU_NMI 3
gpio_set IRQ_BMC_CPU_NMI 0

sgpio_export RST_PCA9548_SENSOR_R_N 5
gpio_set RST_PCA9548_SENSOR_R_N 1 

sgpio_export FM_BMC_CRASHLOG_TRIG_R1_N 7
gpio_set FM_BMC_CRASHLOG_TRIG_R1_N 1 

sgpio_export FM_DEBUG_PORT_PRSNT_N_OUT 9
gpio_set FM_DEBUG_PORT_PRSNT_N_OUT $(gpio_get FM_DEBUG_PORT_PRSNT_N_IN)

sgpio_export IRQ_PCH_SCI_WHEA_R_N 11
gpio_set IRQ_PCH_SCI_WHEA_R_N 1

sgpio_export FM_PECI_SEL_R_N 13
gpio_set FM_PECI_SEL_R_N 1

sgpio_export FM_JTAG_BMC_MUX_SEL_R1 15
gpio_set FM_JTAG_BMC_MUX_SEL_R1 0

sgpio_export FM_SPD_REMOTE_EN_R 17
gpio_set FM_SPD_REMOTE_EN_R 0

#sgpio_export FM_PWR_BTN 19

sgpio_export RST_BMC_RSTBTN_OUT_R_N 21
gpio_set RST_BMC_RSTBTN_OUT_R_N 1

#sgpio_export FM_P12V_HSC_ENABLE_R1 23

#sgpio_export RST_NCT7904D_R1_N 25

sgpio_export IRQ_BMC_PCH_NMI_N 27
gpio_set IRQ_BMC_PCH_NMI_N 1

#sgpio_export FM_RISER1_JTAG_SEL_R_N 29

#sgpio_export FM_RISER2_JTAG_SEL_R_N 31

#sgpio_export RST_RSMRST_R_N 33

sgpio_export FM_BMC_ONCTL_R_N 35
gpio_set FM_BMC_ONCTL_R_N 1

#sgpio_export FM_BMC_SUSACK_R1_N 39

sgpio_export FM_BIOS_DEBUG_EN_R_N 41
gpio_set FM_BIOS_DEBUG_EN_R_N 1

sgpio_export GPU_NVS_PWR_BRAKE_R_N 43
gpio_set GPU_NVS_PWR_BRAKE_R_N 1

sgpio_export GPU_WP_HW_CTRL_R_N 45
gpio_set GPU_WP_HW_CTRL_R_N 1

sgpio_export CLK_GEN2_BMC_RC_OE_N 47
gpio_set CLK_GEN2_BMC_RC_OE_N 0

sgpio_export IRQ_SML0_ALERT_R_N 49
gpio_set IRQ_SML0_ALERT_R_N 1

sgpio_export BMC_MONITER 51
gpio_set BMC_MONITER 0

sgpio_export FM_PFR_DSW_PWROK_N 53
pio_set FM_PFR_DSW_PWROK_N 1

sgpio_export FM_PFR_OVR_RTC_R 55
gpio_set FM_PFR_OVR_RTC_R 0

sgpio_export FM_TPM_PRSNT_0_N 57
gpio_set FM_TPM_PRSNT_0_N 1

sgpio_export FM_TPM_PRSNT_1_N 59
gpio_set FM_TPM_PRSNT_1_N 1

sgpio_export IRQ_PCH_TPM_SPI_N 61
gpio_set IRQ_PCH_TPM_SPI_N 1

sgpio_export USB_OC0_REAR_R_N 63
gpio_set USB_OC0_REAR_R_N 1

sgpio_export GPU_FPGA_RST_N 65
gpio_set GPU_FPGA_RST_N 1

sgpio_export FM_JTAG_TCK_MUX_BMC_SEL 67
gpio_set FM_JTAG_TCK_MUX_BMC_SEL 0

sgpio_export CLK_BUF1_GPU_FPGA_OE_R_N 69
gpio_set CLK_BUF1_GPU_FPGA_OE_R_N 0

sgpio_export FM_BMC_CPU_FBRK_OUT_N 71
gpio_set FM_BMC_CPU_FBRK_OUT_N 1 

#BMC Ready
sgpio_export FM_BMC_READY_R_PLD_N 37
gpio_set FM_BMC_READY_R_PLD_N 1

#devmem_set_bit 0x1e6e24bc 24
#devmem_set_bit 0x1e6e24bc 25

devmem_set_bit 0x1e7800e0 8
devmem_clear_bit 0x1e7000e4 8
