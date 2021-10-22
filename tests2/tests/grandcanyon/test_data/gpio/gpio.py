#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
    # FPGA_CRCERROR, GPIOB0 (824)
    "FPGA_CRCERROR": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # FPGA_NCONFIG, GPIOB1 (825)
    "FPGA_NCONFIG": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_SCC_FAULT_IN_R, GPIOB2 (826)
    "BMC_SCC_FAULT_IN_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # HSC_P12V_DPB_FAULT_N_IN_R, GPIOB4 (828)
    "HSC_P12V_DPB_FAULT_N_IN_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # HSC_COMP_FAULT_N_IN_R, GPIOB5 (829)
    "HSC_COMP_FAULT_N_IN_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_NIC_PWRBRK_R, GPIOC1 (833)
    "BMC_NIC_PWRBRK_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # BMC_NIC_SMRST_N_R, GPIOC2 (834)
    "BMC_NIC_SMRST_N_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # NIC_WAKE_BMC_N, GPIOC3 (835)
    "NIC_WAKE_BMC_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_NIC_FULL_PWR_EN_R, GPIOC4 (836)
    "BMC_NIC_FULL_PWR_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # NIC_FULL_PWR_PG, GPIOC5 (837)
    "NIC_FULL_PWR_PG": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BOARD_REV_ID0, GPIOH0 (872)
    # Will change in different stages
    # thus ignore to check the value
    "BOARD_REV_ID0": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
    },
    # BOARD_REV_ID1, GPIOH1 (873)
    # Will change in different stages
    # thus ignore to check the value
    "BOARD_REV_ID1": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
    },
    # BOARD_REV_ID2, GPIOH2 (874)
    # Will change in different stages
    # thus ignore to check the value
    "BOARD_REV_ID2": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
    },
    # EN_ASD_DEBUG, GPIOI5 (885)
    "EN_ASD_DEBUG": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # DEBUG_RST_BTN_N, GPIOI7 (887)
    "DEBUG_RST_BTN_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "1",
    },
    # E1S_1_P3V3_PG_R, GPIOL6 (910)
    "E1S_1_P3V3_PG_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # E1S_2_P3V3_PG_R, GPIOL7 (911)
    "E1S_2_P3V3_PG_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_FPGA_UART_SEL0_R, GPIOM0 (912)
    "BMC_FPGA_UART_SEL0_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # BMC_FPGA_UART_SEL1_R, GPIOM1 (913)
    "BMC_FPGA_UART_SEL1_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # BMC_FPGA_UART_SEL2_R, GPIOM2 (914)
    "BMC_FPGA_UART_SEL2_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # BMC_FPGA_UART_SEL3_R, GPIOM3 (915)
    "BMC_FPGA_UART_SEL3_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # DEBUG_BMC_UART_SEL_R, GPIOM4 (916)
    "DEBUG_BMC_UART_SEL_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "0",
    },
    # DEBUG_PWR_BTN_N(DEBUG_GPIO_BMC_1), GPION0 (920)
    "DEBUG_PWR_BTN_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "1",
    },
    # DEBUG_GPIO_BMC_2, GPION1 (921)
    "DEBUG_GPIO_BMC_2": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # DEBUG_GPIO_BMC_3, GPION2 (922)
    "DEBUG_GPIO_BMC_3": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # DEBUG_GPIO_BMC_4, GPION3 (923)
    "DEBUG_GPIO_BMC_4": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # DEBUG_GPIO_BMC_5, GPION4 (924)
    "DEBUG_GPIO_BMC_5": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # DEBUG_GPIO_BMC_6, GPION5 (925)
    "DEBUG_GPIO_BMC_6": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # USB_OC_N1, GPIOO3 (931)
    "USB_OC_N1": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # SCC_I2C_EN_R, GPIOO4 (932)
    "SCC_I2C_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_READY_R, GPIOO5 (933)
    "BMC_READY_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # LED_POSTCODE_0, GPIOO7 (935)
    "LED_POSTCODE_0": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # LED_POSTCODE_1, GPIOP0 (936)
    "LED_POSTCODE_1": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # LED_POSTCODE_2, GPIOP1 (937)
    "LED_POSTCODE_2": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # LED_POSTCODE_3, GPIOP2 (938)
    "LED_POSTCODE_3": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # LED_POSTCODE_4, GPIOP3 (939)
    "LED_POSTCODE_4": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # LED_POSTCODE_5, GPIOP4 (940)
    "LED_POSTCODE_5": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # LED_POSTCODE_6, GPIOP5 (941)
    "LED_POSTCODE_6": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # LED_POSTCODE_7, GPIOP6 (942)
    "LED_POSTCODE_7": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_LOC_HEARTBEAT_R, GPIOQ1 (945)
    # Will always change
    # thus ignore to check the value
    "BMC_LOC_HEARTBEAT_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
    },
    # BIC_READY_IN, GPIOQ6 (950)
    "BIC_READY_IN": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # COMP_STBY_PG_IN, GPIOQ7 (951)
    "COMP_STBY_PG_IN": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # UIC_LOC_TYPE_IN, GPIOR1 (953)
    "UIC_LOC_TYPE_IN": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # UIC_RMT_TYPE_IN, GPIOR2 (954)
    "UIC_RMT_TYPE_IN": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # BMC_COMP_PWR_EN_R, GPIOR4 (956)
    "BMC_COMP_PWR_EN_R": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # EXT_MINISAS_INS_OR_N_IN, GPIOR6 (958)
    "EXT_MINISAS_INS_OR_N_IN": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # NIC_PRSNTB3_N
    "NIC_PRSNTB3_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "both",
        "uevent": "",
        "value": "0",
    },
    # FM_BMC_TPM_PRSNT_N, GPIOS2 (962)
    "FM_BMC_TPM_PRSNT_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # DEBUG_CARD_PRSNT_N, GPIOS3 (963)
    "DEBUG_CARD_PRSNT_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_RST_BTN_N_R, GPIOV0 (984)
    "BMC_RST_BTN_N_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # PCIE_COMP_UIC_RST_N, GPIOV1 (985)
    "PCIE_COMP_UIC_RST_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_COMP_SYS_RST_N_R, GPIOV2 (986)
    "BMC_COMP_SYS_RST_N_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_PWRGD_NIC, GPIOV3 (987)
    "BMC_PWRGD_NIC": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_LED_STATUS_BLUE_EN_R, GPIOV4 (988)
    "BMC_LED_STATUS_BLUE_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_LED_STATUS_YELLOW_EN_R, GPIOV5 (989)
    "BMC_LED_STATUS_YELLOW_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    # BMC_LED_PWR_BTN_EN_R, GPIOV6 (990)
    "BMC_LED_PWR_BTN_EN_R": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    # BMC_UIC_LOCATION_IN, GPIOY3 (1011)
    "BMC_UIC_LOCATION_IN": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
}
