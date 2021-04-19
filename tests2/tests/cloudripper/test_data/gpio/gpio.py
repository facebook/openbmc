#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

GPIOS = {
    "NP_POWER_STABLE_3V3_R": {
        "active_low": "0",
        "direction": "in",
    },
    "SMB_CPLD_HITLESS_R": {
        "active_low": "0",
        "direction": "out",
    },
    "BMC_EMMC_CD_N": {
        "active_low": "0",
        "direction": "in",
    },
    "BMC_EMMC_WP_N": {
        "active_low": "0",
        "direction": "in",
    },
    "CPLD_INT_BMC_R": {
        "active_low": "0",
        "direction": "in",
    },
    "CPLD_RESERVED_2_R": {
        "active_low": "0",
        "direction": "in",
    },
    "BMC_DOM_FPGA2_RST_L_R": {
        "active_low": "0",
        "direction": "out",
    },
    "FCM_CARD_PRESENT": {
        "active_low": "0",
        "direction": "in",
    },
    "GPIO_H0": {
        "active_low": "0",
        "direction": "in",
    },
    "GPIO_H1": {
        "active_low": "0",
        "direction": "in",
    },
    "GPIO_H2": {
        "active_low": "0",
        "direction": "in",
    },
    "GPIO_H3": {
        "active_low": "0",
        "direction": "in",
    },
    "GPIO_H4": {
        "active_low": "0",
        "direction": "in",
    },
    "GPIO_H5": {
        "active_low": "0",
        "direction": "in",
    },
    "BMC_UART_SEL_5": {
        "active_low": "0",
        "direction": "in",
    },
    "SCM_CPLD_HITLESS_R": {
        "active_low": "0",
        "direction": "out",
    },
    "PWR_CPLD_JTAG_EN_N": {
        "active_low": "0",
        "direction": "out",
    },
    "BMC_JTAG_MUX_IN_R": {
        "active_low": "0",
        "direction": "out",
    },
    "FCM_CPLD_JTAG_EN_N": {
        "active_low": "0",
        "direction": "out",
    },
    "SYS_CPLD_JTAG_EN_N": {
        "active_low": "0",
        "direction": "out",
    },
    "DOM_FPGA1_JTAG_EN_N_R": {
        "active_low": "0",
        "direction": "out",
    },
    "GPIO_H6": {
        "active_low": "0",
        "direction": "in",
    },
    "SCM_CPLD_JTAG_EN_N": {
        "active_low": "0",
        "direction": "out",
    },
    "GPIO_H7": {
        "active_low": "0",
        "direction": "in",
    },
    "BMC_UART_SEL_2": {
        "active_low": "0",
        "direction": "in",
    },
    "BMC_CPLD_SPARE3_R": {
        "active_low": "0",
        "direction": "in",
    },
    "FCM_CPLD_HITLESS_R": {
        "active_low": "0",
        "direction": "out",
    },
    "PWR_CPLD_HITLESS_R": {
        "active_low": "0",
        "direction": "out",
    },
    "BMC_IN_P1220_R": {
        "active_low": "0",
        "direction": "in",
    },
    "BMC_FCM_SEL_R": {
        "active_low": "0",
        "direction": "out",
    },
    "BMC_CPLD_SPARE4_R": {
        "active_low": "0",
        "direction": "in",
    },
    "SYS_CPLD_RST_L_R": {
        "active_low": "0",
        "direction": "out",
    },
    "BMC_CPLD_SPARE2_R": {
        "active_low": "0",
        "direction": "in",
    },
    "DOM_FPGA2_JTAG_EN_N_R": {
        "active_low": "0",
        "direction": "out",
    },
    "BMC_CPLD_SPARE1_R": {
        "active_low": "0",
        "direction": "out",
    },
    "DEBUG_PRESENT_N": {
        "active_low": "0",
        "direction": "in",
    },
    "BMC_SCM_CPLD_EN_R": {
        "active_low": "0",
        "direction": "out",
    },
    "BMC_CPLD_TPM_RST_L_R": {
        "active_low": "0",
        "direction": "out",
    },
    "BMC_DOM_FPGA1_RST_L_R": {
        "active_low": "0",
        "direction": "out",
    },
    "CPLD_RESERVED_3_R": {
        "active_low": "0",
        "direction": "in",
    },
    "CPLD_RESERVED_4_R": {
        "active_low": "0",
        "direction": "in",
    },
    "BMC_P1220_SYS_OK": {
        "active_low": "0",
        "direction": "in",
    },
    "BMC_POWER_OK": {
        "active_low": "0",
        "direction": "in",
    },
    "SCM_LPC_AD_0": {
        "active_low": "0",
        "direction": "in",
    },
    "SCM_LPC_AD_1": {
        "active_low": "0",
        "direction": "in",
    },
    "SCM_LPC_AD_2": {
        "active_low": "0",
        "direction": "in",
    },
    "SCM_LPC_AD_3": {
        "active_low": "0",
        "direction": "out",
    },
    "SCM_LPC_CLK": {
        "active_low": "0",
        "direction": "in",
    },
    "SCM_LPC_FRAME_N": {
        "active_low": "0",
        "direction": "out",
    },
    "SCM_LPC_SERIRQ_N": {
        "active_low": "0",
        "direction": "in",
    },
    "BMC_LPCRST_N": {
        "active_low": "0",
        "direction": "out",
    },
    "BMC_FPGA_JTAG_EN": {
        "active_low": "0",
        "direction": "out",
    },
    "CPU_CATERR_MSMI_R": {
        "active_low": "0",
        "direction": "in",
    },
    "SYS_CPLD_INT_L_R": {
        "active_low": "0",
        "direction": "in",
    },
}
