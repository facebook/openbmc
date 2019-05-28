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
# python galaxy100_gpio_parser.py data/galaxy100-BMC-gpio.csv
# DO NOT MODIFY THE TABLE!!!
# Manual modification will be overridden!!!

board_gpio_table_v1 = [
    BoardGPIO("GPIOA2", "BMC_GPIOA2"),
    BoardGPIO("GPIOA3", "BMC_GPIOA3"),
    BoardGPIO("GPIOB0", "UNNAMED_45_AST2400A1TFBGA408_I1"),
    BoardGPIO("GPIOB4", "LPC_RST_N"),
    BoardGPIO("GPIOC2", "BMC_GPIOC2"),
    BoardGPIO("GPIOD0", "BMC_GPIOD0"),
    BoardGPIO("GPIOD1", "BMC_GPIOD1"),
    BoardGPIO("GPIOD2", "BMC_GPIOD2"),
    BoardGPIO("GPIOD3", "BMC_GPIOD3"),
    BoardGPIO("GPIOD4", "BMC_GPIOD4"),
    BoardGPIO("GPIOD5", "BMC_GPIOD5"),
    BoardGPIO("GPIOD6", "VBUS_DET"),
    BoardGPIO("GPIOE0", "UNNAMED_46_AST2400A1TFBGA408_I1"),
    BoardGPIO("GPIOE2", "UNNAMED_46_AST2400A1TFBGA408__1"),
    BoardGPIO("GPIOE4", "BUF_PRSNT_SCM_N"),
    BoardGPIO("GPIOF1", "UNNAMED_46_AST2400A1TFBGA408__2"),
    BoardGPIO("GPIOF2", "UNNAMED_46_AST2400A1TFBGA408__3"),
    BoardGPIO("GPIOF3", "UNNAMED_46_AST2400A1TFBGA408__4"),
    BoardGPIO("GPIOG4", "UNNAMED_47_AST2400A1TFBGA408__5"),
    BoardGPIO("GPIOG5", "UNNAMED_47_AST2400A1TFBGA408__6"),
    BoardGPIO("GPIOH0", "LC1_SLOT_ID1"),
    BoardGPIO("GPIOH1", "LC1_SLOT_ID2"),
    BoardGPIO("GPIOH2", "LC1_SLOT_ID3"),
    BoardGPIO("GPIOH4", "UNNAMED_46_AST2400A1TFBGA408_18"),
    BoardGPIO("GPIOH5", "UNNAMED_46_AST2400A1TFBGA408_19"),
    BoardGPIO("GPIOH6", "UNNAMED_46_AST2400A1TFBGA408_20"),
    BoardGPIO("GPIOL0", "UNNAMED_46_AST2400A1TFBGA408__6"),
    BoardGPIO("GPIOL1", "UNNAMED_46_AST2400A1TFBGA408__7"),
    BoardGPIO("GPIOL2", "UNNAMED_46_AST2400A1TFBGA408__8"),
    BoardGPIO("GPIOL3", "UNNAMED_46_AST2400A1TFBGA408__9"),
    BoardGPIO("GPIOM1", "UNNAMED_46_AST2400A1TFBGA408_11"),
    BoardGPIO("GPIOO2", "VCP_ID_LC1_0"),
    BoardGPIO("GPIOO3", "VCP_ID_LC1_1"),
    BoardGPIO("GPIOO4", "BUF_PRESENT_FC_CON_1"),
    BoardGPIO("GPIOO5", "BUF_PRESENT_FC_CON_2"),
    BoardGPIO("GPIOO6", "BUF_PRESENT_FC_CON_3"),
    BoardGPIO("GPIOO7", "BUF_PRESENT_FC_CON_4"),
    BoardGPIO("GPIOP7", "BMC_READY_N_R"),
    BoardGPIO("GPIOQ4", "BMC_GPIOQ4"),
    BoardGPIO("GPIOQ5", "SYSCPLD_INT_1"),
    BoardGPIO("GPIOQ6", "SYSCPLD_INT_2"),
    BoardGPIO("GPIOQ7", "BMC_GPIOQ7"),
    BoardGPIO("GPIOR6", "SWITCH_MDC_R"),
    BoardGPIO("GPIOR7", "SWITCH_MDIO_R"),
    BoardGPIO("GPIOS4", "BMC_CPLD_GPIO4"),
    BoardGPIO("GPIOS5", "BMC_CPLD_GPIO5"),
    BoardGPIO("GPIOW0", "LM57_VTEMP"),
]
