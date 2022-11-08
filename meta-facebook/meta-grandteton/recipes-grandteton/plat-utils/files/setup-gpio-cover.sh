#!/bin/bash
#
# Copyright 2022-present Facebook. All Rights Reserved.
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
# Provides:          gpio-cover
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set up GPIO pins as appropriate can be cover
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions



# ==SGPIO Input Cover==
sgpio_export FM_OCP_SFF_PWR_GOOD_R 0
sgpio_export IRQ_SML1_PMBUS_PLD_ALERT_N 2
sgpio_export PWRGD_PVNN_MAIN_CPU0 4
sgpio_export PWRGD_PVPP_HBM_CPU1 6

sgpio_export FM_ADR_ACK_R 10
sgpio_export IRQ_PVCCD_CPU0_VRHOT_LVC3_N 12
sgpio_export FM_DIS_PS_PWROK_DLY 14
sgpio_export IRQ_PSYS_CRIT_N 16
sgpio_export IRQ_PVCCD_CPU1_VRHOT_LVC3_N 18

sgpio_export FM_PCH_PLD_GLB_RST_WARN_N 22

sgpio_export FM_PCH_BMC_THERMTRIP_N 28
sgpio_export FM_PCHHOT_R_N 30

sgpio_export FM_PCH_PRSNT_N 34
sgpio_export IRQ_HSC_FAULT_N 36
sgpio_export FM_OCP_V3_2_PWR_GOOD_R 38

sgpio_export PWRGD_PVNN_PCH_AUX 44

sgpio_export PWRGD_PVNN_MAIN_CPU1 48
sgpio_export PWRGD_PVPP_HBM_CPU0 50

sgpio_export PWRGD_P3V3 56
sgpio_export PWRGD_PVCCFA_EHV_FIVRA_CPU0 58

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
sgpio_export IRQ_OC_DETECT_N  178
sgpio_export PWRGD_FAIL_CPU0_GH_R 180
sgpio_export H_CPU_RMCA_LVC2_R2_N 182
#sgpio_export H_CPU_CATERR_LVC2_R2_N 184
sgpio_export H_CPU1_PROCHOT_LVC1_N 186
sgpio_export IRQ_UV_DETECT_N 188

sgpio_export FM_ME_AUTHN_FAIL 192
sgpio_export PRSNT_SWB_R_N 194
sgpio_export IRQ_PSU_ALERT_R_N 196
sgpio_export FM_THROTTLE_R_N 198
sgpio_export RST_PLTRST_PLD_N 200
sgpio_export H_CPU0_PROCHOT_LVC1_N 202
sgpio_export FM_PCH_BEEP_LED 204
sgpio_export FM_HDD_LED_N 206
sgpio_export FM_PFR_NO_SERVICE_ACT_N 208
sgpio_export FM_PFR_UPDATE_N 210

sgpio_export CPLD_SGPIO_READY_ID0 248
sgpio_export CPLD_SGPIO_READY_ID1 250
sgpio_export CPLD_SGPIO_READY_ID2 252
sgpio_export CPLD_SGPIO_READY_ID3 254






# == SGPIO OUT Cover==
sgpio_export FM_ESPI_PLD_R1_EN 1
gpio_set FM_ESPI_PLD_R1_EN 1

sgpio_export IRQ_BMC_CPU_NMI 3
gpio_set IRQ_BMC_CPU_NMI 0

sgpio_export RST_PCA9548_SENSOR_R_N 5
gpio_set RST_PCA9548_SENSOR_R_N 1

sgpio_export FM_BMC_CRASHLOG_TRIG_R1_N 7
gpio_set FM_BMC_CRASHLOG_TRIG_R1_N 1

sgpio_export FM_DEBUG_PORT_PRSNT_N_OUT 9
gpio_set FM_DEBUG_PORT_PRSNT_N_OUT "$(gpio_get FM_DEBUG_PORT_PRSNT_N_IN)"

sgpio_export FM_PECI_SEL_R_N 13
gpio_set FM_PECI_SEL_R_N 1

#sgpio_export FM_JTAG_BMC_MUX_SEL 15
#gpio_set FM_JTAG_BMC_MUX_SEL 0

#sgpio_export FM_PWR_BTN 19

sgpio_export BMC_READY_N 23
gpio_set BMC_READY_N 1

sgpio_export TEST_SGPIO_EVENT_LOG 25
gpio_set TEST_SGPIO_EVENT_LOG 0

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

sgpio_export CLK_GEN2_BMC_RC_OE_N 47
gpio_set CLK_GEN2_BMC_RC_OE_N 0

sgpio_export IRQ_SML0_ALERT_R_N 49
gpio_set IRQ_SML0_ALERT_R_N 1

sgpio_export FM_PFR_DSW_PWROK_N 53
pio_set FM_PFR_DSW_PWROK_N 1

sgpio_export FM_PFR_OVR_RTC_R 55
gpio_set FM_PFR_OVR_RTC_R 0

sgpio_export FM_TPM_PRSNT_0_N 57
gpio_set FM_TPM_PRSNT_0_N 1

sgpio_export FM_TPM_PRSNT_1_N 59
gpio_set FM_TPM_PRSNT_1_N 1

sgpio_export IRQ_PCH_TPM_SPI_OUT 61
gpio_set IRQ_PCH_TPM_SPI_OUT 1

sgpio_export USB_OC0_REAR_R_OUT 63
gpio_set USB_OC0_REAR_R_OUT 1

sgpio_export GPU_FPGA_RST_N 65
gpio_set GPU_FPGA_RST_N 0

sgpio_export FM_JTAG_TCK_MUX_BMC_SEL 67
gpio_set FM_JTAG_TCK_MUX_BMC_SEL 0

sgpio_export CLK_BUF1_GPU_FPGA_OE_R_N 69
gpio_set CLK_BUF1_GPU_FPGA_OE_R_N 0

sgpio_export FM_BMC_CPU_FBRK_OUT_N 71
gpio_set FM_BMC_CPU_FBRK_OUT_N 1

sgpio_export GPIO_READY 37
gpio_set GPIO_READY 1

