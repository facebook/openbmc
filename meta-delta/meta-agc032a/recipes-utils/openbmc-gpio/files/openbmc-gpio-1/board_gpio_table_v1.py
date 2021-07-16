# Copyright 2019-present Facebook. All rights reserved.
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

# The following table is generated using:
# python openbmc_gpio_parser.py data/agc032a-bmc-gpio-evta.csv
# DO NOT MODIFY THE TABLE!!!
# Manual modification will be overridden!!!

board_gpio_table_v1_th3 = [
    BoardGPIO("GPIOA3", "BMC_EMMC_RST_N_R"),
    BoardGPIO("GPIOAA0", "DEBUG_PRESENT_N"),
    BoardGPIO("GPIOAA1", "SYS_CPLD_RST_N"),
    BoardGPIO("GPIOAA2", "SYS_CPLD_INT_N"),
    BoardGPIO("GPIOAA7", "BMC_SCM_CPLD_EN"),
    BoardGPIO("GPIOAB0", "CPLD_INT_BMC"),
    BoardGPIO("GPIOAB1", "CPLD_RESERVED_2"),
    BoardGPIO("GPIOAB2", "CPLD_RESERVED_3"),
    BoardGPIO("GPIOAB3", "CPLD_RESERVED_4"),
    BoardGPIO("GPIOAC5", "SCM_LPC_FRAME_N"),
    BoardGPIO("GPIOAC6", "SCM_LPC_SERIRQ_N"),
    BoardGPIO("GPIOAC7", "BMC_LPCRST_N"),
    BoardGPIO("GPIOE2", "TH3_POWER_OK"),
    BoardGPIO("GPIOF4", "CPU_CATERR_MSMI"),
    BoardGPIO("GPIOG0", "BMC_JTAG_MUX_IN"),
    BoardGPIO("GPIOG1", "DOM_FPGA1_JTAG_EN_N"),
    BoardGPIO("GPIOG2", "DOM_FPGA2_JTAG_EN_N"),
    BoardGPIO("GPIOH0", "GPIO_H0"),
    BoardGPIO("GPIOH1", "GPIO_H1"),
    BoardGPIO("GPIOH2", "GPIO_H2"),
    BoardGPIO("GPIOH3", "GPIO_H3"),
    BoardGPIO("GPIOH4", "GPIO_H4"),
    BoardGPIO("GPIOH5", "GPIO_H5"),
    BoardGPIO("GPIOH6", "GPIO_H6"),
    BoardGPIO("GPIOH7", "GPIO_H7"),
    BoardGPIO("GPIOI4", "BMC_SPI1_CS0"),
    BoardGPIO("GPIOI5", "BMC_SPI1_CLK"),
    BoardGPIO("GPIOI6", "BMC_SPI1_MOSI"),
    BoardGPIO("GPIOI7", "BMC_SPI1_MISO"),
    BoardGPIO("GPIOJ0", "PWR_CPLD_JTAG_EN_N"),
    BoardGPIO("GPIOJ1", "SYS_CPLD_JTAG_EN_N"),
    BoardGPIO("GPIOJ2", "SCM_CPLD_JTAG_EN_N"),
    BoardGPIO("GPIOJ3", "FCM_CPLD_JTAG_EN_N"),
    BoardGPIO("GPIOM0", "BMC_CPLD_SPARE1"),
    BoardGPIO("GPIOM1", "BMC_CPLD_SPARE2"),
    BoardGPIO("GPIOM2", "BMC_CPLD_SPARE3"),
    BoardGPIO("GPIOM3", "BMC_CPLD_SPARE4"),
    BoardGPIO("GPION0", "RMON_PF_1"),
    BoardGPIO("GPION1", "RMON_PF_2"),
    BoardGPIO("GPION2", "RMON_PF_3"),
    BoardGPIO("GPION3", "RMON_RF_1"),
    BoardGPIO("GPION4", "RMON_RF_2"),
    BoardGPIO("GPION5", "RMON_RF_3"),
    BoardGPIO("GPIOO1", "BMC_FCM_SEL"),
    BoardGPIO("GPIOO6", "PWR_CPLD_HITLESS"),
    BoardGPIO("GPIOO7", "FCM_CPLD_HITLESS"),
    BoardGPIO("GPIOP0", "SCM_CPLD_HITLESS"),
    BoardGPIO("GPIOP1", "SMB_CPLD_HITLESS"),
    BoardGPIO("GPIOP2", "BMC_DOM_FPGA1_RST"),
    BoardGPIO("GPIOP3", "BMC_DOM_FPGA2_RST"),
    BoardGPIO("GPIOS2", "BMC_P1220_SYS_OK"),
    BoardGPIO("GPIOS3", "BMC_POWER_OK"),
    BoardGPIO("GPIOS4", "BMC_UART_SEL2"),
    BoardGPIO("GPIOS5", "BMC_UART_SEL5"),
    BoardGPIO("GPIOZ0", "BMC_SPI_1_WP_N"),
    BoardGPIO("GPIOZ1", "BMC_SPI_2_WP_N"),
    BoardGPIO("GPIOZ2", "BMC_SPI_1_RST"),
    BoardGPIO("GPIOZ3", "BMC_SPI_2_RST"),
    BoardGPIO("GPIOZ5", "BMC_IN_P1220"),
]

board_gpio_table_v1_gb = [
    BoardGPIO("GPIOA3", "BMC_EMMC_RST_N_R"),
    BoardGPIO("GPIOAA0", "DEBUG_PRESENT_N"),
    BoardGPIO("GPIOAA1", "SYS_CPLD_RST_N"),
    BoardGPIO("GPIOAA2", "SYS_CPLD_INT_N"),
    BoardGPIO("GPIOAA7", "BMC_SCM_CPLD_EN"),
    BoardGPIO("GPIOAB0", "CPLD_INT_BMC"),
    BoardGPIO("GPIOAB1", "CPLD_RESERVED_2"),
    BoardGPIO("GPIOAB2", "CPLD_RESERVED_3"),
    BoardGPIO("GPIOAB3", "CPLD_RESERVED_4"),
    BoardGPIO("GPIOAC5", "SCM_LPC_FRAME_N"),
    BoardGPIO("GPIOAC6", "SCM_LPC_SERIRQ_N"),
    BoardGPIO("GPIOAC7", "BMC_LPCRST_N"),
    BoardGPIO("GPIOE2", "GB_POWER_OK"),
    BoardGPIO("GPIOF4", "CPU_CATERR_MSMI"),
    BoardGPIO("GPIOG0", "BMC_JTAG_MUX_IN"),
    BoardGPIO("GPIOG1", "DOM_FPGA1_JTAG_EN_N"),
    BoardGPIO("GPIOG2", "DOM_FPGA2_JTAG_EN_N"),
    BoardGPIO("GPIOH0", "GPIO_H0"),
    BoardGPIO("GPIOH1", "GPIO_H1"),
    BoardGPIO("GPIOH2", "GPIO_H2"),
    BoardGPIO("GPIOH3", "GPIO_H3"),
    BoardGPIO("GPIOH4", "GPIO_H4"),
    BoardGPIO("GPIOH5", "GPIO_H5"),
    BoardGPIO("GPIOH6", "GPIO_H6"),
    BoardGPIO("GPIOH7", "GPIO_H7"),
    BoardGPIO("GPIOI4", "BMC_SPI1_CS0"),
    BoardGPIO("GPIOI5", "BMC_SPI1_CLK"),
    BoardGPIO("GPIOI6", "BMC_SPI1_MOSI"),
    BoardGPIO("GPIOI7", "BMC_SPI1_MISO"),
    BoardGPIO("GPIOJ0", "PWR_CPLD_JTAG_EN_N"),
    BoardGPIO("GPIOJ1", "SYS_CPLD_JTAG_EN_N"),
    BoardGPIO("GPIOJ2", "SCM_CPLD_JTAG_EN_N"),
    BoardGPIO("GPIOJ3", "FCM_CPLD_JTAG_EN_N"),
    BoardGPIO("GPIOM0", "BMC_CPLD_SPARE1"),
    BoardGPIO("GPIOM1", "BMC_CPLD_SPARE2"),
    BoardGPIO("GPIOM2", "BMC_CPLD_SPARE3"),
    BoardGPIO("GPIOM3", "BMC_CPLD_SPARE4"),
    BoardGPIO("GPION0", "RMON_PF_1"),
    BoardGPIO("GPION1", "RMON_PF_2"),
    BoardGPIO("GPION2", "RMON_PF_3"),
    BoardGPIO("GPION3", "RMON_RF_1"),
    BoardGPIO("GPION4", "RMON_RF_2"),
    BoardGPIO("GPION5", "RMON_RF_3"),
    BoardGPIO("GPIOO1", "BMC_FCM_SEL"),
    BoardGPIO("GPIOO6", "PWR_CPLD_HITLESS"),
    BoardGPIO("GPIOO7", "FCM_CPLD_HITLESS"),
    BoardGPIO("GPIOP0", "SCM_CPLD_HITLESS"),
    BoardGPIO("GPIOP1", "SMB_CPLD_HITLESS"),
    BoardGPIO("GPIOP2", "BMC_DOM_FPGA1_RST"),
    BoardGPIO("GPIOP3", "BMC_DOM_FPGA2_RST"),
    BoardGPIO("GPIOS2", "BMC_P1220_SYS_OK"),
    BoardGPIO("GPIOS3", "BMC_POWER_OK"),
    BoardGPIO("GPIOS4", "BMC_UART_SEL2"),
    BoardGPIO("GPIOS5", "BMC_UART_SEL5"),
    BoardGPIO("GPIOZ0", "BMC_SPI_1_WP_N"),
    BoardGPIO("GPIOZ1", "BMC_SPI_2_WP_N"),
    BoardGPIO("GPIOZ2", "BMC_SPI_1_RST"),
    BoardGPIO("GPIOZ3", "BMC_SPI_2_RST"),
    BoardGPIO("GPIOZ5", "BMC_IN_P1220"),
]

board_gpio_table_v1_delta = [
    BoardGPIO("GPIOAA0", "BMC_TCK"),  #BMC_TCK
    BoardGPIO("GPIOAA1", "BMC_TMS"),  # BMC_TMS
    BoardGPIO("GPIOAA2", "BMC_TDI"),  # BMC_TDI
    BoardGPIO("GPIOAA3", "BMC_TDO"),  # BMC_TDO
    BoardGPIO("GPIOH0", "BMC_CPLD_UPDATE_EN"),  #BMC_CPLD_UPDATE_EN
    BoardGPIO("GPIOH1", "CONSOLE_SEL_BMC"),     #RJ45_CONSOLE_SEL_BMC
    BoardGPIO("GPIOM5", "CPU_ALERT"),           #CPU_ALERT
    BoardGPIO("GPIOM6", "BMC_RST_CPU"),         #BMC_RST_CPU
    BoardGPIO("GPIOM7", "RTC_RESET"),           #RTC_RESETn
]
