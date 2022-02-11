# Copyright 2015-present Facebook. All rights reserved.
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

from openbmc_gpio_table import BoardGPIO


# The fallowing table is generated using:
# python wedge100_gpio_parser.py data/wedge100-BMC-gpio.csv
# DO NOT MODIFY THE TABLE!!!
# Manual modification will be overridden!!!

board_gpio_table_v2 = [
    BoardGPIO('GPIOB0', 'PANTHER_I2C_ALERT_N'),
    BoardGPIO('GPIOB1', 'MSERV_NIC_SMBUS_ALERT_N'),
    BoardGPIO('GPIOB2', 'DEBUG_PORT_UART_SEL_N'),
    BoardGPIO('GPIOB5', 'LED_POSTCODE_5'),
    BoardGPIO('GPIOB6', 'LED_POSTCODE_6'),
    BoardGPIO('GPIOB7', 'LED_POSTCODE_7'),
    BoardGPIO('GPIOC2', 'ISO_BUF_EN'),
    BoardGPIO('GPIOD0', 'BMC_PWR_BTN_IN_N'),
    BoardGPIO('GPIOD1', 'BMC_PWR_BTN_OUT_N'),
    BoardGPIO('GPIOD2', 'BMC_CPLD_RESET1'),
    BoardGPIO('GPIOD3', 'BMC_CPLD_RESET2'),
    BoardGPIO('GPIOD4', 'BMC_CPLD_RESET3'),
    BoardGPIO('GPIOD5', 'BMC_CPLD_RESET4'),
    BoardGPIO('GPIOD7', 'BMC_CPLD_QSFP_INT'),
    BoardGPIO('GPIOE0', 'DEBUG_UART_SEL_0'),
    BoardGPIO('GPIOE2', 'SWITCH_EEPROM1_WRT'),
    BoardGPIO('GPIOE4', 'ISO_MICROSRV_PRSNT_N'),
    BoardGPIO('GPIOE5', 'LED_PWR_BLUE'),
    BoardGPIO('GPIOF0', 'MSERVE_ISOBUF_EN'),
    BoardGPIO('GPIOF1', 'BMC_MAIN_RESET_N'),
    BoardGPIO('GPIOF2', 'CPLD_JTAG_SEL'),
    BoardGPIO('GPIOF3', 'UPPER_CPLD_JTAG_SEL'),
    BoardGPIO('GPIOF4', 'MSERV_POWERUP'),
    BoardGPIO('GPIOG0', 'LED_POSTCODE_0'),
    BoardGPIO('GPIOG1', 'LED_POSTCODE_1'),
    BoardGPIO('GPIOG2', 'LED_POSTCODE_2'),
    BoardGPIO('GPIOG3', 'LED_POSTCODE_3'),
    BoardGPIO('GPIOG4', 'BMC_WDTRST1'),
    BoardGPIO('GPIOG5', 'BMC_WDTRST2'),
    BoardGPIO('GPIOG6', 'BMC_COM_NIC_ISOBUF_EN'),
    BoardGPIO('GPIOG7', 'BMC_COM_PANTHER_ISOBUF_EN'),
    BoardGPIO('GPIOH3', 'QSFP_LED_POSITION'),
    BoardGPIO('GPIOH4', 'PM_SM_ALERT_N'),
    BoardGPIO('GPIOI4', 'BMC_EEPROM1_SPI_SS'),
    BoardGPIO('GPIOI5', 'BMC_EEPROM1_SPI_SCK'),
    BoardGPIO('GPIOI6', 'BMC_EEPROM1_SPI_MOSI'),
    BoardGPIO('GPIOI7', 'BMC_EEPROM1_SPI_MISO'),
    BoardGPIO('GPIOJ0', 'RCKMON_SPARE0'),
    BoardGPIO('GPIOJ1', 'RCKMON_SPARE1'),
    BoardGPIO('GPIOJ2', 'RCKMON_SPARE2'),
    BoardGPIO('GPIOJ3', 'RCKMON_SPARE3'),
    BoardGPIO('GPIOJ4', 'FANCARD_CPLD_TMS'),
    BoardGPIO('GPIOJ5', 'FANCARD_CPLD_TCK'),
    BoardGPIO('GPIOJ6', 'FANCARD_CPLD_TDI'),
    BoardGPIO('GPIOJ7', 'FANCARD_CPLD_TDO'),
    BoardGPIO('GPIOL5', 'BMC_UART_1_RTS'),
    BoardGPIO('GPIOM0', 'CPLD_UPD_EN'),
    BoardGPIO('GPIOM1', 'SMB_ALERT'),
    BoardGPIO('GPIOM3', 'TH_POWERUP'),
    BoardGPIO('GPIOM4', 'BMC_CPLD_TMS'),
    BoardGPIO('GPIOM5', 'BMC_CPLD_TDI'),
    BoardGPIO('GPIOM6', 'BMC_CPLD_TCK'),
    BoardGPIO('GPIOM7', 'BMC_CPLD_TDO'),
    BoardGPIO('GPIOS3', 'BMC_UPPER_CPLD_TMS'),
    BoardGPIO('GPIOH0', 'BMC_UPPER_CPLD_TDI'),
    BoardGPIO('GPIOH1', 'BMC_UPPER_CPLD_TCK'),
    BoardGPIO('GPIOH2', 'BMC_UPPER_CPLD_TDO'),
    BoardGPIO('GPION0', 'ISO_COM_SUS_S3_N'),
    BoardGPIO('GPION1', 'ISO_COM_SUS_S4_N'),
    BoardGPIO('GPION2', 'ISO_COM_SUS_S5_N'),
    BoardGPIO('GPION3', 'ISO_COM_SUS_STAT_N'),
    BoardGPIO('GPION4', 'ISO_COM_BRG_WDT'),
    BoardGPIO('GPION5', 'BRG_COM_BIOS_DIS0_N'),
    BoardGPIO('GPION6', 'BRG_COM_BIOS_DIS1_N'),
    BoardGPIO('GPION7', 'ISO_COM_PWROK'),
    BoardGPIO('GPIOO0', 'COM_SPI_SEL'),
    BoardGPIO('GPIOO1', 'COM6_BUF_EN'),
    BoardGPIO('GPIOO2', 'RCKMON_SPARE4'),
    BoardGPIO('GPIOO3', 'RCKMON_SPARE5'),
    BoardGPIO('GPIOO4', 'TPM_SPI_SEL'),
    BoardGPIO('GPIOO5', 'TPM_SPI_BUF_EN'),
    BoardGPIO('GPIOO6', 'ISO_BRG_THRMTRIP_N'),
    BoardGPIO('GPIOO7', 'ISO_BRG_THRM_N'),
    BoardGPIO('GPIOP0', 'RMON1_PF'),
    BoardGPIO('GPIOP1', 'RMON1_RF'),
    BoardGPIO('GPIOP2', 'RMON2_PF'),
    BoardGPIO('GPIOP3', 'RMON2_RF'),
    BoardGPIO('GPIOP4', 'RMON3_PF'),
    BoardGPIO('GPIOP5', 'RMON3_RF'),
    BoardGPIO('GPIOP6', 'FANCARD_I2C_ALARM'),
    BoardGPIO('GPIOP7', 'BMC_READY_N'),
    BoardGPIO('GPIOQ4', 'BMC_CPLD_POWER_INT'),
    BoardGPIO('GPIOQ5', 'BMC_CPLD_SPARE7'),
    BoardGPIO('GPIOQ6', 'USB_OCS_N1'),
    BoardGPIO('GPIOQ7', 'BMC_HEARTBEAT_N'),
    BoardGPIO('GPIOR0', 'SPI_IBMC_BT_CS1_N_R'),
    BoardGPIO('GPIOR6', 'SWITCH_MDC'),
    BoardGPIO('GPIOR7', 'SWITCH_MDIO'),
    BoardGPIO('GPIOS0', 'BMC_SPI_WP_N'),
    BoardGPIO('GPIOY3', 'LED_POSTCODE_4'),
]
