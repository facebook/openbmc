# Copyright 2018-present Facebook. All rights reserved.
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
# python minipack_gpio_parser.py data/minipack-BMC-gpio.csv
# DO NOT MODIFY THE TABLE!!!
# Manual modification will be overridden!!!

board_gpio_table_v2 = [
    BoardGPIO("GPIOA2", "BMC_SPI1_CS1"),
    BoardGPIO("GPIOA3", "BMC_eMMC_RST_N", "high"),
    BoardGPIO("GPIOAA2", "BMC_SCM_CPLD_TMS"),
    BoardGPIO("GPIOAA3", "BMC_SCM_CPLD_TCK"),
    BoardGPIO("GPIOAA4", "BMC_SCM_CPLD_TDI"),
    BoardGPIO("GPIOAA5", "BMC_SCM_CPLD_TDO"),
    BoardGPIO("GPIOAA7", "BMC_SCM_CPLD_EN"),
    BoardGPIO("GPIOAB0", "CPLD_INT_BMC"),
    BoardGPIO("GPIOE2", "TH3_PWR_OK"),
    BoardGPIO("GPIOF0", "BMC10_9548_1_RST", "high"),
    BoardGPIO("GPIOF4", "CPU_CATERR_MSMI"),
    BoardGPIO("GPIOG0", "BMC_CPLD_TDI"),
    BoardGPIO("GPIOG1", "BMC_CPLD_TCK"),
    BoardGPIO("GPIOG2", "BMC_CPLD_TMS"),
    BoardGPIO("GPIOG3", "BMC_CPLD_TDO"),
    BoardGPIO("GPIOG7", "BMC_SPI1_CS2"),
    BoardGPIO("GPIOH0", "GPIOH0", "low"),
    BoardGPIO("GPIOH1", "GPIOH1", "low"),
    BoardGPIO("GPIOH2", "GPIOH2", "low"),
    BoardGPIO("GPIOH3", "GPIOH3", "low"),
    BoardGPIO("GPIOH4", "GPIOH4", "low"),
    BoardGPIO("GPIOH5", "GPIOH5", "low"),
    BoardGPIO("GPIOH6", "GPIOH6", "low"),
    BoardGPIO("GPIOH7", "GPIOH7", "low"),
    BoardGPIO("GPIOI4", "BMC_SPI1_CS0"),
    BoardGPIO("GPIOI5", "BMC_SPI1_CLK"),
    BoardGPIO("GPIOI6", "BMC_SPI1_MOSI"),
    BoardGPIO("GPIOI7", "BMC_SPI1_MISO"),
    BoardGPIO("GPIOJ4", "BMC_FCM_CPLD_TMS"),
    BoardGPIO("GPIOJ5", "BMC_FCM_CPLD_TCK"),
    BoardGPIO("GPIOJ6", "BMC_FCM_CPLD_TDI"),
    BoardGPIO("GPIOJ7", "BMC_FCM_CPLD_TDO"),
    BoardGPIO("GPIOM0", "BMC_CPLD_GPIO_SPARE1"),
    BoardGPIO("GPIOM1", "BMC_CPLD_GPIO_SPARE2"),
    BoardGPIO("GPIOM2", "BMC_CPLD_GPIO_SPARE3"),
    BoardGPIO("GPIOM3", "BMC_CPLD_GPIO_SPARE4"),
    BoardGPIO("GPIOM4", "BMC_CPLD_GPIO_SPARE5"),
    BoardGPIO("GPION0", "BMC_PIM1_9548_RST_N", "high"),
    BoardGPIO("GPION1", "BMC_PIM2_9548_RST_N", "high"),
    BoardGPIO("GPION2", "BMC_PIM3_9548_RST_N", "high"),
    BoardGPIO("GPION3", "BMC_PIM4_9548_RST_N", "high"),
    BoardGPIO("GPION4", "BMC_PIM5_9548_RST_N", "high"),
    BoardGPIO("GPION5", "BMC_PIM6_9548_RST_N", "high"),
    BoardGPIO("GPION6", "BMC_PIM7_9548_RST_N", "high"),
    BoardGPIO("GPION7", "BMC_PIM8_9548_RST_N", "high"),
    BoardGPIO("GPIOO0", "BMC_FCM_T_MUX_SEL"),
    BoardGPIO("GPIOO1", "BMC_SYSCPLD_JTAG_MUX_SEL"),
    BoardGPIO("GPIOO3", "BMC_FCM_B_MUX_SEL"),
    BoardGPIO("GPIOO4", "SWITCH_EEPROM1_WRT"),
    BoardGPIO("GPIOP0", "SCM_CPLD_HITLESS"),
    BoardGPIO("GPIOP1", "SMB_CPLD_HITLESS"),
    BoardGPIO("GPIOP2", "BMC_RST_FPGA", "high"),
    BoardGPIO("GPIOR6", "BMC_SW_MDC1"),
    BoardGPIO("GPIOR7", "BMC_SW_MDIO1"),
    BoardGPIO("GPIOS1", "BMC_BCM5396_MUX_SEL"),
    BoardGPIO("GPIOS2", "BMC_P1220_SYS_OK"),
    BoardGPIO("GPIOS3", "BMC_PWR_OK"),
    BoardGPIO("GPIOS4", "BMC_UART_SEL2"),
    BoardGPIO("GPIOS5", "BMC_UART_SEL5"),
    BoardGPIO("GPIOS6", "BMC9_9548_0_RST", "high"),
    BoardGPIO("GPIOS7", "BMC12_9548_2_RST", "high"),
    BoardGPIO("GPIOZ0", "BMC_SPI_1_WP_N"),
    BoardGPIO("GPIOZ1", "BMC_SPI_2_WP_N"),
    BoardGPIO("GPIOZ4", "SCM_USB_PRSNT"),
    BoardGPIO("GPIOZ5", "BMC_IN_P1220"),
]
