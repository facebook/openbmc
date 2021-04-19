#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
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
    "BMC_COM_NIC_ISOBUF_EN": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_COM_PANTHER_ISOBUF_EN": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_CPLD_POWER_INT": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_CPLD_QSFP_INT": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_CPLD_RESET1": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_CPLD_RESET2": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_CPLD_RESET3": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_CPLD_RESET4": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_CPLD_SPARE7": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_CPLD_TCK": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_CPLD_TDI": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_CPLD_TDO": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_CPLD_TMS": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_HEARTBEAT_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_MAIN_RESET_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_PWR_BTN_IN_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_PWR_BTN_OUT_N": {"active_low": "0", "direction": "out", "edge": "none"},
    "BMC_READY_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_SPI_WP_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_UART_1_RTS": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_WDTRST1": {"active_low": "0", "direction": "in", "edge": "none"},
    "BMC_WDTRST2": {"active_low": "0", "direction": "in", "edge": "none"},
    "BOARD_REV_ID0": {"active_low": "0", "direction": "in", "edge": "none"},
    "BOARD_REV_ID1": {"active_low": "0", "direction": "in", "edge": "none"},
    "BOARD_REV_ID2": {"active_low": "0", "direction": "in", "edge": "none"},
    "BRG_COM_BIOS_DIS0_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "BRG_COM_BIOS_DIS1_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "COM6_BUF_EN": {"active_low": "0", "direction": "out", "edge": "none"},
    "COM_SPI_SEL": {"active_low": "0", "direction": "out", "edge": "none"},
    "CPLD_JTAG_SEL": {"active_low": "0", "direction": "in", "edge": "none"},
    "CPLD_UPD_EN": {"active_low": "0", "direction": "in", "edge": "none"},
    "DEBUG_PORT_UART_SEL_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "DEBUG_UART_SEL_0": {"active_low": "0", "direction": "out", "edge": "none"},
    "FANCARD_CPLD_TCK": {"active_low": "0", "direction": "in", "edge": "none"},
    "FANCARD_CPLD_TDI": {"active_low": "0", "direction": "in", "edge": "none"},
    "FANCARD_CPLD_TDO": {"active_low": "0", "direction": "in", "edge": "none"},
    "FANCARD_CPLD_TMS": {"active_low": "0", "direction": "in", "edge": "none"},
    "FANCARD_I2C_ALARM": {"active_low": "0", "direction": "in", "edge": "none"},
    "ISO_BRG_THRMTRIP_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "ISO_BRG_THRM_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "ISO_BUF_EN": {"active_low": "0", "direction": "in", "edge": "none"},
    "ISO_COM_BRG_WDT": {"active_low": "0", "direction": "in", "edge": "none"},
    "ISO_COM_PWROK": {"active_low": "0", "direction": "in", "edge": "none"},
    "ISO_COM_SUS_S3_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "ISO_COM_SUS_S4_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "ISO_COM_SUS_S5_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "ISO_COM_SUS_STAT_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "ISO_MICROSRV_PRSNT_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "LED_POSTCODE_0": {"active_low": "0", "direction": "in", "edge": "none"},
    "LED_POSTCODE_1": {"active_low": "0", "direction": "in", "edge": "none"},
    "LED_POSTCODE_2": {"active_low": "0", "direction": "in", "edge": "none"},
    "LED_POSTCODE_3": {"active_low": "0", "direction": "in", "edge": "none"},
    "LED_POSTCODE_4": {"active_low": "0", "direction": "in", "edge": "none"},
    "LED_POSTCODE_5": {"active_low": "0", "direction": "in", "edge": "none"},
    "LED_POSTCODE_6": {"active_low": "0", "direction": "in", "edge": "none"},
    "LED_POSTCODE_7": {"active_low": "0", "direction": "in", "edge": "none"},
    "LED_PWR_BLUE": {"active_low": "0", "direction": "in", "edge": "none"},
    "MSERVE_ISOBUF_EN": {"active_low": "0", "direction": "in", "edge": "none"},
    "MSERV_NIC_SMBUS_ALERT_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "MSERV_POWERUP": {"active_low": "0", "direction": "in", "edge": "none"},
    "PANTHER_I2C_ALERT_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "PM_SM_ALERT_N": {"active_low": "0", "direction": "in", "edge": "none"},
    "QSFP_LED_POSITION": {"active_low": "0", "direction": "in", "edge": "none"},
    "RCKMON_SPARE0": {"active_low": "0", "direction": "in", "edge": "none"},
    "RCKMON_SPARE1": {"active_low": "0", "direction": "in", "edge": "none"},
    "RCKMON_SPARE2": {"active_low": "0", "direction": "in", "edge": "none"},
    "RCKMON_SPARE3": {"active_low": "0", "direction": "in", "edge": "none"},
    "RCKMON_SPARE4": {"active_low": "0", "direction": "in", "edge": "none"},
    "RCKMON_SPARE5": {"active_low": "0", "direction": "in", "edge": "none"},
    "RMON1_PF": {"active_low": "0", "direction": "in", "edge": "none"},
    "RMON1_RF": {"active_low": "0", "direction": "in", "edge": "none"},
    "RMON2_PF": {"active_low": "0", "direction": "in", "edge": "none"},
    "RMON2_RF": {"active_low": "0", "direction": "in", "edge": "none"},
    "RMON3_PF": {"active_low": "0", "direction": "in", "edge": "none"},
    "RMON3_RF": {"active_low": "0", "direction": "in", "edge": "none"},
    "SMB_ALERT": {"active_low": "0", "direction": "in", "edge": "none"},
    "SWITCH_MDC": {"active_low": "0", "direction": "in", "edge": "none"},
    "SWITCH_MDIO": {"active_low": "0", "direction": "in", "edge": "none"},
    "TH_POWERUP": {"active_low": "0", "direction": "in", "edge": "none"},
    "TPM_SPI_BUF_EN": {"active_low": "0", "direction": "in", "edge": "none"},
    "TPM_SPI_SEL": {"active_low": "0", "direction": "in", "edge": "none"},
    "USB_OCS_N1": {"active_low": "0", "direction": "in", "edge": "none"},
}
