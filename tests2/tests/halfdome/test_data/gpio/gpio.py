#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
    "AC_ON_OFF_BTN_BMC_SLOT1_N_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "AC_ON_OFF_BTN_BMC_SLOT3_N_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "BMC_READY_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "BOARD_ID0": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "BOARD_ID1": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "BOARD_ID2": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "BOARD_ID3": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "DUAL_FAN0_DETECT_BMC_N_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "DUAL_FAN1_DETECT_BMC_N_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "EMMC_PRESENT_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "EMMC_RST_N_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FAN0_BMC_CPLD_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FAN1_BMC_CPLD_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FAN2_BMC_CPLD_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FAN3_BMC_CPLD_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FAST_PROCHOT_BMC_N_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FM_BMC_SLOT1_ISOLATED_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FM_BMC_SLOT2_ISOLATED_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FM_BMC_SLOT3_ISOLATED_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FM_BMC_SLOT4_ISOLATED_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FM_BMC_TPM_PRSNT_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "FM_HSC_BMC_FAULT_N_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FM_NIC_WAKE_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "FM_PWRBRK_PRIMARY_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "FM_RESBTN_SLOT1_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "1",
    },
    "FM_RESBTN_SLOT3_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "1",
    },
    "HSC_FAULT_BMC_SLOT1_N_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "1",
    },
    "HSC_FAULT_BMC_SLOT3_N_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "1",
    },
    "HSC_FAULT_SLOT2_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "1",
    },
    "HSC_FAULT_SLOT4_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "1",
    },
    "NIC_POWER_BMC_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "OCP_DEBUG_BMC_PRSNT_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "OCP_NIC_PRSNT_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "0",
    },
    "P12V_NIC_FAULT_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "P3V3_NIC_FAULT_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "PRSNT_MB_BMC_SLOT1_BB_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "0",
    },
    "PRSNT_MB_BMC_SLOT3_BB_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "0",
    },
    "PWRGD_NIC_BMC": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "PWROK_STBY_BMC_SLOT1_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "PWROK_STBY_BMC_SLOT2": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "PWROK_STBY_BMC_SLOT3_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "PWROK_STBY_BMC_SLOT4": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "RST_BMC_USB_HUB_N_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "RST_BMC_WDRST2_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "RST_PCIE_RESET_SLOT1_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "RST_PCIE_RESET_SLOT2_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "RST_PCIE_RESET_SLOT3_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "RST_PCIE_RESET_SLOT4_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SLOT1_ID0_DETECT_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SLOT1_ID1_DETECT_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SLOT2_ID0_DETECT_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SLOT2_ID1_DETECT_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "SLOT3_ID0_DETECT_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "SLOT3_ID1_DETECT_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SLOT4_ID0_DETECT_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "SLOT4_ID1_DETECT_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "SMB_BMC_SLOT1_ALT_R_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SMB_BMC_SLOT2_ALT_R_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SMB_BMC_SLOT3_ALT_R_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SMB_BMC_SLOT4_ALT_R_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SMB_HOTSWAP_BMC_ALERT_N_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SMB_RST_PRIMARY_BMC_N_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SMB_RST_SECONDARY_BMC_N_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "USB_BMC_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
}
