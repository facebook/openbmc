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





from openbmc_gpio_table import (
    BitsEqual, BitsNotEqual, And, Or, Function)


# The fallowing table is generated using:
# python ast_gpio_parser.py data/ast2400-gpio.csv
# DO NOT MODIFY THE TABLE!!!
# Manual modification will be overridden!!!

soc_gpio_table = {
    'A1': [
        Function('SD1WP#', BitsEqual(0x90, [0], 0x1)),
        Function('SDA13', BitsEqual(0x90, [26], 0x1)),
        Function('GPIOC7', None)
    ],
    'A10': [
        Function('GPIOU0', BitsEqual(0xa0, [8], 0x1)),
        Function('RMII2TXD0', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2TXD0', None)
    ],
    'A11': [
        Function('GPIOV0', BitsEqual(0xa0, [16], 0x1)),
        Function('RMII1CRSDV', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXD2', None)
    ],
    'A12': [
        Function('GPIOT0', BitsEqual(0xa0, [0], 0x1)),
        Function('RMII1TXEN', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1TXCK', None)
    ],
    'A13': [
        Function('GPIOT5', BitsEqual(0xa0, [5], 0x1)),
        Function('UNDEFINED5', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1TXD3', None)
    ],
    'A14': [
        Function('SGPSCK', BitsEqual(0x84, [0], 0x1)),
        Function('GPIOG0', None)
    ],
    'A15': [
        Function('NRI3', BitsEqual(0x80, [19], 0x1)),
        Function('GPIE2(Out)', Or(BitsEqual(0x8c, [13], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE3', None)
    ],
    'A16': [
        Function('SD2CD#', BitsEqual(0x90, [1], 0x1)),
        Function('GPID6(In)', Or(BitsEqual(0x8c, [11], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD6', None)
    ],
    'A17': [
        Function('SD2DAT1', BitsEqual(0x90, [1], 0x1)),
        Function('GPID2(Out)', Or(BitsEqual(0x8c, [9], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD3', None)
    ],
    'A18': [
        Function('SD2CLK', BitsEqual(0x90, [1], 0x1)),
        Function('GPID0(In)', Or(BitsEqual(0x8c, [8], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD0', None)
    ],
    'A19': [
        Function('NRTS4', BitsEqual(0x80, [29], 0x1)),
        Function('SIOSCI#', Or(BitsEqual(0xa4, [15], 0x1), BitsEqual(0x70, [19], 0x0))),
        Function('GPIOF5', None)
    ],
    'A2': [
        Function('SD1DAT0', BitsEqual(0x90, [0], 0x1)),
        Function('SCL11', BitsEqual(0x90, [24], 0x1)),
        Function('GPIOC2', None)
    ],
    'A20': [
        Function('NDSR4', BitsEqual(0x80, [26], 0x1)),
        Function('SIOPWRGD', Or(BitsEqual(0xa4, [13], 0x1), BitsEqual(0x70, [19], 0x0))),
        Function('GPIOF2', None)
    ],
    'A3': [
        Function('MDC2', BitsEqual(0x90, [2], 0x1)),
        Function('TIMER7', BitsEqual(0x80, [6], 0x1)),
        Function('GPIOA6', None)
    ],
    'A4': [
        Function('TIMER3', BitsEqual(0x80, [2], 0x1)),
        Function('GPIOA2', None)
    ],
    'A5': [
        Function('MDIO1', BitsEqual(0x88, [31], 0x1)),
        Function('GPIOR7', None)
    ],
    'A6': [
        Function('ROMD14', Or(BitsEqual(0x90, [6], 0x1), BitsEqual(0x70, [4, 1, 0], 0x4))),
        Function('TXD6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH6', None)
    ],
    'A7': [
        Function('ROMD11', Or(BitsEqual(0x90, [6], 0x1), BitsEqual(0x70, [4, 1, 0], 0x4))),
        Function('NRI6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH3', None)
    ],
    'A8': [
        Function('ROMD8', Or(BitsEqual(0x90, [6], 0x1), BitsEqual(0x70, [4, 1, 0], 0x4))),
        Function('NCTS6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH0', None)
    ],
    'A9': [
        Function('GPIOV4', BitsEqual(0xa0, [20], 0x1)),
        Function('RMII2RXD0', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXD0', None)
    ],
    'AA1': [
        Function('VPIB8', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [30], 0x1))),
        Function('TXD2', BitsEqual(0x84, [30], 0x1)),
        Function('GPIOM6', None)
    ],
    'AA2': [
        Function('VPIG2', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x88, [2], 0x1))),
        Function('PWM2', BitsEqual(0x88, [2], 0x1)),
        Function('GPION2', None)
    ],
    'AA22': [
        Function('FLBUSY#', BitsEqual(0x84, [6], 0x1)),
        Function('GPIOG6', None)
    ],
    'AA3': [
        Function('VPIG6', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [6], 0x1))),
        Function('PWM6', BitsEqual(0x88, [6], 0x1)),
        Function('GPION6', None)
    ],
    'AA4': [
        Function('VPIR0', And(BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x88, [10], 0x1))),
        Function('GPIOO2/TACH2', None)
    ],
    'AA5': [
        Function('VPIR3', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [13], 0x1))),
        Function('GPIOO5/TACH5', None)
    ],
    'AA6': [
        Function('VPIR9', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [19], 0x1))),
        Function('GPIOP3/TACH11', None)
    ],
    'AA7': [
        Function('BMCINT', BitsEqual(0x88, [22], 0x1)),
        Function('GPIOP6/TACH14', None)
    ],
    'AB1': [
        Function('VPIG3', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x88, [3], 0x1))),
        Function('PWM3', BitsEqual(0x88, [3], 0x1)),
        Function('GPION3', None)
    ],
    'AB2': [
        Function('VPIG7', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [7], 0x1))),
        Function('PWM7', BitsEqual(0x88, [7], 0x1)),
        Function('GPION7', None)
    ],
    'AB20': [
        Function('USB2_HDN', BitsEqual(0x90, [29], 0x1)),
        Function('USB2_DN', None)
    ],
    'AB21': [
        Function('USB2_HDP', BitsEqual(0x90, [29], 0x1)),
        Function('USB2_DP', None)
    ],
    'AB3': [
        Function('VPIR1', And(BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x88, [11], 0x1))),
        Function('GPIOO3/TACH3', None)
    ],
    'AB4': [
        Function('VPIR4', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [14], 0x1))),
        Function('GPIOO6/TACH6', None)
    ],
    'AB5': [
        Function('VPIR7', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [17], 0x1))),
        Function('GPIOP1/TACH9', None)
    ],
    'AB7': [
        Function('FLACK', BitsEqual(0x88, [23], 0x1)),
        Function('GPIOP7/TACH15', None)
    ],
    'B1': [
        Function('SCL4', BitsEqual(0x90, [17], 0x1)),
        Function('GPIOQ2', None)
    ],
    'B10': [
        Function('GPIOU1', BitsEqual(0xa0, [9], 0x1)),
        Function('RMII2TXD1', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2TXD1', None)
    ],
    'B11': [
        Function('GPIOU7', BitsEqual(0xa0, [15], 0x1)),
        Function('RMII1RXD1', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXD1', None)
    ],
    'B12': [
        Function('GPIOT1', BitsEqual(0xa0, [1], 0x1)),
        Function('UNDEFINED3', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1TXCTL', None)
    ],
    'B13': [
        Function('OSCCLK', BitsEqual(0x2c, [1], 0x1)),
        Function('WDTRST1', BitsEqual(0x84, [4], 0x1)),
        Function('GPIOG4', None)
    ],
    'B14': [
        Function('RXD3', BitsEqual(0x80, [23], 0x1)),
        Function('GPIE6(Out)', Or(BitsEqual(0x8c, [15], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE7', None)
    ],
    'B15': [
        Function('NDSR3', BitsEqual(0x80, [18], 0x1)),
        Function('GPIE2(In)', Or(BitsEqual(0x8c, [13], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE2', None)
    ],
    'B16': [
        Function('SD2DAT3', BitsEqual(0x90, [1], 0x1)),
        Function('GPID4(Out)', Or(BitsEqual(0x8c, [10], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD5', None)
    ],
    'B17': [
        Function('SD2DAT0', BitsEqual(0x90, [1], 0x1)),
        Function('GPID2(In)', Or(BitsEqual(0x8c, [9], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD2', None)
    ],
    'B18': [
        Function('NDTR4', BitsEqual(0x80, [28], 0x1)),
        Function('GPIOF4', None)
    ],
    'B19': [
        Function('NDCD4', BitsEqual(0x80, [25], 0x1)),
        Function('SIOPBI#', Or(BitsEqual(0xa4, [12], 0x1), BitsEqual(0x70, [19], 0x0))),
        Function('GPIOF1', None)
    ],
    'B2': [
        Function('SD1CD#', BitsEqual(0x90, [0], 0x1)),
        Function('SCL13', BitsEqual(0x90, [26], 0x1)),
        Function('GPIOC6', None)
    ],
    'B22': [
        Function('SPICS0#', BitsNotEqual(0x70, [13, 12], 0x0)),
        Function('VBCS#', BitsEqual(0x70, [5], 0x1)),
        Function('GPIOI4', None)
    ],
    'B3': [
        Function('SD1CMD', BitsEqual(0x90, [0], 0x1)),
        Function('SDA10', BitsEqual(0x90, [23], 0x1)),
        Function('GPIOC1', None)
    ],
    'B4': [
        Function('SDA9', BitsEqual(0x90, [22], 0x1)),
        Function('TIMER6', BitsEqual(0x80, [5], 0x1)),
        Function('GPIOA5', None)
    ],
    'B5': [
        Function('MAC2LINK', BitsEqual(0x80, [1], 0x1)),
        Function('GPIOA1', None)
    ],
    'B6': [
        Function('ROMD13', Or(BitsEqual(0x90, [6], 0x1), BitsEqual(0x70, [4, 1, 0], 0x4))),
        Function('NRTS6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH5', None)
    ],
    'B7': [
        Function('ROMD10', Or(BitsEqual(0x90, [6], 0x1), BitsEqual(0x70, [4, 1, 0], 0x4))),
        Function('NDSR6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH2', None)
    ],
    'B9': [
        Function('GPIOV3', BitsEqual(0xa0, [19], 0x1)),
        Function('UNDEFINED10', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXCTL', None)
    ],
    'C1': [
        Function('SCL6', BitsEqual(0x90, [19], 0x1)),
        Function('GPIOK2', None)
    ],
    'C10': [
        Function('GPIOU2', BitsEqual(0xa0, [10], 0x1)),
        Function('UNDEFINED7', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2TXD2', None)
    ],
    'C11': [
        Function('GPIOU6', BitsEqual(0xa0, [14], 0x1)),
        Function('RMII1RXD0', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXD0', None)
    ],
    'C12': [
        Function('GPIOT2', BitsEqual(0xa0, [2], 0x1)),
        Function('RMII1TXD0', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1TXD0', None)
    ],
    'C13': [
        Function('SGPSI1', BitsEqual(0x84, [3], 0x1)),
        Function('GPIOG3', None)
    ],
    'C14': [
        Function('TXD3', BitsEqual(0x80, [22], 0x1)),
        Function('GPIE6(In)', Or(BitsEqual(0x8c, [15], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE6', None)
    ],
    'C15': [
        Function('NDCD3', BitsEqual(0x80, [17], 0x1)),
        Function('GPIE0(Out)', Or(BitsEqual(0x8c, [12], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE1', None)
    ],
    'C16': [
        Function('SD2DAT2', BitsEqual(0x90, [1], 0x1)),
        Function('GPID4(In)', Or(BitsEqual(0x8c, [10], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD4', None)
    ],
    'C17': [
        Function('RXD4', BitsEqual(0x80, [31], 0x1)),
        Function('GPIOF7', None)
    ],
    'C18': [
        Function('SPIDO', BitsNotEqual(0x70, [13, 12], 0x0)),
        Function('VBDO', BitsEqual(0x70, [5], 0x1)),
        Function('GPIOI6', None)
    ],
    'C2': [
        Function('SDA3', BitsEqual(0x90, [16], 0x1)),
        Function('GPIOQ1', None)
    ],
    'C20': [
        Function('SYSDI', BitsEqual(0x70, [13], 0x1)),
        Function('GPIOI3', None)
    ],
    'C21': [
        Function('SIOS3#', Or(BitsEqual(0xa4, [8], 0x1), BitsEqual(0x70, [19], 0x0))),
        Function('GPIOY0', None)
    ],
    'C22': [
        Function('SYSCS#', BitsEqual(0x70, [13], 0x1)),
        Function('GPIOI0', None)
    ],
    'C3': [
        Function('SD1DAT3', BitsEqual(0x90, [0], 0x1)),
        Function('SDA12', BitsEqual(0x90, [25], 0x1)),
        Function('GPIOC5', None)
    ],
    'C4': [
        Function('SD1CLK', BitsEqual(0x90, [0], 0x1)),
        Function('SCL10', BitsEqual(0x90, [23], 0x1)),
        Function('GPIOC0', None)
    ],
    'C5': [
        Function('SCL9', BitsEqual(0x90, [22], 0x1)),
        Function('TIMER5', BitsEqual(0x80, [4], 0x1)),
        Function('GPIOA4', None)
    ],
    'C6': [
        Function('MDC1', BitsEqual(0x88, [30], 0x1)),
        Function('GPIOR6', None)
    ],
    'C7': [
        Function('ROMD9', Or(BitsEqual(0x90, [6], 0x1), BitsEqual(0x70, [4, 1, 0], 0x4))),
        Function('NDCD6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH1', None)
    ],
    'C8': [
        Function('GPIOV7', BitsEqual(0xa0, [23], 0x1)),
        Function('RMII2RXER', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXD3', None)
    ],
    'C9': [
        Function('GPIOV2', BitsEqual(0xa0, [18], 0x1)),
        Function('RMII2RCLK', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXCK', None)
    ],
    'D1': [
        Function('SDA7', BitsEqual(0x90, [20], 0x1)),
        Function('GPIOK5', None)
    ],
    'D10': [
        Function('GPIOU3', BitsEqual(0xa0, [11], 0x1)),
        Function('UNDEFINED8', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2TXD3', None)
    ],
    'D11': [
        Function('GPIOU5', BitsEqual(0xa0, [13], 0x1)),
        Function('UNDEFINED9', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXCTL', None)
    ],
    'D12': [
        Function('GPIOT3', BitsEqual(0xa0, [3], 0x1)),
        Function('RMII1TXD1', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1TXD1', None)
    ],
    'D13': [
        Function('SGPSI0', BitsEqual(0x84, [2], 0x1)),
        Function('GPIOG2', None)
    ],
    'D14': [
        Function('NRTS3', BitsEqual(0x80, [21], 0x1)),
        Function('GPIE4(Out)', Or(BitsEqual(0x8c, [14], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE5', None)
    ],
    'D15': [
        Function('NCTS3', BitsEqual(0x80, [16], 0x1)),
        Function('GPIE0(In)', Or(BitsEqual(0x8c, [12], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE0', None)
    ],
    'D16': [
        Function('SD2CMD', BitsEqual(0x90, [1], 0x1)),
        Function('GPID0(Out)', Or(BitsEqual(0x8c, [8], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD1', None)
    ],
    'D17': [
        Function('NRI4', BitsEqual(0x80, [27], 0x1)),
        Function('SIOPBO#', Or(BitsEqual(0xa4, [14], 0x1), BitsEqual(0x70, [19], 0x0))),
        Function('GPIOF3', None)
    ],
    'D18': [
        Function('NCTS4', BitsEqual(0x80, [24], 0x1)),
        Function('GPIOF0', None)
    ],
    'D19': [
        Function('SYSDO', BitsEqual(0x70, [13], 0x1)),
        Function('GPIOI2', None)
    ],
    'D2': [
        Function('SDA5', BitsEqual(0x90, [18], 0x1)),
        Function('GPIOK1', None)
    ],
    'D3': [
        Function('SCL3', BitsEqual(0x90, [16], 0x1)),
        Function('GPIOQ0', None)
    ],
    'D4': [
        Function('SD1DAT2', BitsEqual(0x90, [0], 0x1)),
        Function('SCL12', BitsEqual(0x90, [25], 0x1)),
        Function('GPIOC4', None)
    ],
    'D5': [
        Function('MDIO2', BitsEqual(0x90, [2], 0x1)),
        Function('TIMER8', BitsEqual(0x80, [7], 0x1)),
        Function('GPIOA7', None)
    ],
    'D6': [
        Function('MAC1LINK', BitsEqual(0x80, [0], 0x1)),
        Function('GPIOA0', None)
    ],
    'D7': [
        Function('ROMD12', Or(BitsEqual(0x90, [6], 0x1), BitsEqual(0x70, [4, 1, 0], 0x4))),
        Function('NDTR6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH4', None)
    ],
    'D8': [
        Function('GPIOV6', BitsEqual(0xa0, [22], 0x1)),
        Function('RMII2CRSDV', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXD2', None)
    ],
    'D9': [
        Function('GPIOT6', BitsEqual(0xa0, [6], 0x1)),
        Function('RMII2TXEN', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2TXCK', None)
    ],
    'E10': [
        Function('GPIOV1', BitsEqual(0xa0, [17], 0x1)),
        Function('RMII1RXER', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXD3', None)
    ],
    'E11': [
        Function('GPIOU4', BitsEqual(0xa0, [12], 0x1)),
        Function('RMII1RCLK', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXCK', None)
    ],
    'E12': [
        Function('GPIOT4', BitsEqual(0xa0, [4], 0x1)),
        Function('UNDEFINED4', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1TXD2', None)
    ],
    'E13': [
        Function('SGPSLD', BitsEqual(0x84, [1], 0x1)),
        Function('GPIOG1', None)
    ],
    'E14': [
        Function('NDTR3', BitsEqual(0x80, [20], 0x1)),
        Function('GPIE4(In)', Or(BitsEqual(0x8c, [14], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE4', None)
    ],
    'E15': [
        Function('SD2WP#', BitsEqual(0x90, [1], 0x1)),
        Function('GPID6(Out)', Or(BitsEqual(0x8c, [11], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD7', None)
    ],
    'E16': [
        Function('TXD4', BitsEqual(0x80, [30], 0x1)),
        Function('GPIOF6', None)
    ],
    'E18': [
        Function('EXTRST#', And(BitsEqual(0x80, [15], 0x1), And(BitsEqual(0x90, [31], 0x0), BitsEqual(0x3c, [3], 0x1)))),
        Function('SPICS1#', And(BitsEqual(0x80, [15], 0x1), BitsEqual(0x90, [31], 0x1))),
        Function('GPIOB7', None)
    ],
    'E19': [
        Function('LPCRST#', Or(BitsEqual(0x80, [12], 0x1), BitsEqual(0x70, [14], 0x1))),
        Function('GPIOB4', None)
    ],
    'E2': [
        Function('SCL7', BitsEqual(0x90, [20], 0x1)),
        Function('GPIOK4', None)
    ],
    'E20': [
        Function('SPIDI', BitsNotEqual(0x70, [13, 12], 0x0)),
        Function('VBDI', BitsEqual(0x70, [5], 0x1)),
        Function('GPIOI7', None)
    ],
    'E3': [
        Function('SCL5', BitsEqual(0x90, [18], 0x1)),
        Function('GPIOK0', None)
    ],
    'E5': [
        Function('SD1DAT1', BitsEqual(0x90, [0], 0x1)),
        Function('SDA11', BitsEqual(0x90, [24], 0x1)),
        Function('GPIOC3', None)
    ],
    'E6': [
        Function('TIMER4', BitsEqual(0x80, [3], 0x1)),
        Function('GPIOA3', None)
    ],
    'E7': [
        Function('ROMD15', Or(BitsEqual(0x90, [6], 0x1), BitsEqual(0x70, [4, 1, 0], 0x4))),
        Function('RXD6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH7', None)
    ],
    'E8': [
        Function('GPIOV5', BitsEqual(0xa0, [21], 0x1)),
        Function('RMII2RXD1', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXD1', None)
    ],
    'E9': [
        Function('GPIOT7', BitsEqual(0xa0, [7], 0x1)),
        Function('UNDEFINED6', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2TXCTL', None)
    ],
    'F18': [
        Function('SALT4', BitsEqual(0x80, [11], 0x1)),
        Function('GPIOB3', None)
    ],
    'F20': [
        Function('SIOS5#', Or(BitsEqual(0xa4, [9], 0x1), BitsEqual(0x70, [19], 0x0))),
        Function('GPIOY1', None)
    ],
    'F3': [
        Function('SDA8', BitsEqual(0x90, [21], 0x1)),
        Function('GPIOK7', None)
    ],
    'F4': [
        Function('SDA6', BitsEqual(0x90, [19], 0x1)),
        Function('GPIOK3', None)
    ],
    'F5': [
        Function('SDA4', BitsEqual(0x90, [17], 0x1)),
        Function('GPIOQ3', None)
    ],
    'G18': [
        Function('SYSCK', BitsEqual(0x70, [13], 0x1)),
        Function('GPIOI1', None)
    ],
    'G19': [
        Function('SPICK', BitsNotEqual(0x70, [13, 12], 0x0)),
        Function('VBCK', BitsEqual(0x70, [5], 0x1)),
        Function('GPIOI5', None)
    ],
    'G20': [
        Function('SIOPWREQ#', Or(BitsEqual(0xa4, [10], 0x1), BitsEqual(0x70, [19], 0x0))),
        Function('GPIOY2', None)
    ],
    'G5': [
        Function('SCL8', BitsEqual(0x90, [21], 0x1)),
        Function('GPIOK6', None)
    ],
    'H1': [
        Function('UNDEFINED2', BitsEqual(0x90, [28], 0x1)),
        Function('GPIOQ7', None)
    ],
    'H18': [
        Function('SALT3', BitsEqual(0x80, [10], 0x1)),
        Function('GPIOB2', None)
    ],
    'H2': [
        Function('UNDEFINED1', BitsEqual(0x90, [28], 0x1)),
        Function('GPIOQ6', None)
    ],
    'H20': [
        Function('LPCPME#', BitsEqual(0x80, [14], 0x1)),
        Function('GPIOB6', None)
    ],
    'H3': [
        Function('SDA14', BitsEqual(0x90, [27], 0x1)),
        Function('GPIOQ5', None)
    ],
    'H4': [
        Function('SCL14', BitsEqual(0x90, [27], 0x1)),
        Function('GPIOQ4', None)
    ],
    'J20': [
        Function('SALT2', BitsEqual(0x80, [9], 0x1)),
        Function('GPIOB1', None)
    ],
    'J21': [
        Function('SALT1', BitsEqual(0x80, [8], 0x1)),
        Function('GPIOB0', None)
    ],
    'J3': [
        Function('SGPMI', BitsEqual(0x84, [11], 0x1)),
        Function('GPIOJ3', None)
    ],
    'J4': [
        Function('SGPMLD', BitsEqual(0x84, [9], 0x1)),
        Function('GPIOJ1', None)
    ],
    'J5': [
        Function('SGPMCK', BitsEqual(0x84, [8], 0x1)),
        Function('GPIOJ0', None)
    ],
    'K18': [
        Function('ROMA23', And(BitsEqual(0x8c, [7], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOR5', And(BitsEqual(0x8c, [7], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOS7', None)
    ],
    'K20': [
        Function('SIOONCTRL#', Or(BitsEqual(0xa4, [11], 0x1), BitsEqual(0x70, [19], 0x0))),
        Function('GPIOY3', None)
    ],
    'K3': [
        Function('UB11_HDN2', BitsEqual(0x90, [3], 0x1)),
        Function('UB11_DN', None)
    ],
    'K4': [
        Function('UB11_HDP2', BitsEqual(0x90, [3], 0x1)),
        Function('UB11_DP', None)
    ],
    'K5': [
        Function('SGPMO', BitsEqual(0x84, [10], 0x1)),
        Function('GPIOJ2', None)
    ],
    'L1': [
        Function('GPIW4', BitsEqual(0xa0, [28], 0x1)),
        Function('ADC4', None)
    ],
    'L18': [
        Function('ROMA10', And(BitsEqual(0xa4, [24], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOG0', And(BitsEqual(0xa4, [24], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOAA0', None)
    ],
    'L19': [
        Function('ROMA11', And(BitsEqual(0xa4, [25], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOG1', And(BitsEqual(0xa4, [25], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOAA1', None)
    ],
    'L2': [
        Function('GPIW3', BitsEqual(0xa0, [27], 0x1)),
        Function('ADC3', None)
    ],
    'L20': [
        Function('ROMA12', And(BitsEqual(0xa4, [26], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOG2', And(BitsEqual(0xa4, [26], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOAA2', None)
    ],
    'L21': [
        Function('ROMA13', And(BitsEqual(0xa4, [27], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOG3', And(BitsEqual(0xa4, [27], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOAA3', None)
    ],
    'L22': [
        Function('ROMA22', And(BitsEqual(0x8c, [6], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOR4', And(BitsEqual(0x8c, [6], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOS6', None)
    ],
    'L3': [
        Function('GPIW2', BitsEqual(0xa0, [26], 0x1)),
        Function('ADC2', None)
    ],
    'L4': [
        Function('GPIW1', BitsEqual(0xa0, [25], 0x1)),
        Function('ADC1', None)
    ],
    'L5': [
        Function('GPIW0', BitsEqual(0xa0, [24], 0x1)),
        Function('ADC0', None)
    ],
    'M1': [
        Function('GPIX1', BitsEqual(0xa4, [1], 0x1)),
        Function('ADC9', None)
    ],
    'M18': [
        Function('ROMA19', And(BitsEqual(0xa8, [1], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOR1', And(BitsEqual(0xa8, [1], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOAB1', None)
    ],
    'M19': [
        Function('ROMA17', And(BitsEqual(0xa4, [31], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOG7', And(BitsEqual(0xa4, [31], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOAA7', None)
    ],
    'M2': [
        Function('GPIX0', BitsEqual(0xa4, [0], 0x1)),
        Function('ADC8', None)
    ],
    'M20': [
        Function('ROMA18', And(BitsEqual(0xa8, [0], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOR0', And(BitsEqual(0xa8, [0], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOAB0', None)
    ],
    'M21': [
        Function('ROMA8', And(BitsEqual(0xa4, [22], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOB6', And(BitsEqual(0xa4, [22], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOZ6', None)
    ],
    'M22': [
        Function('ROMA9', And(BitsEqual(0xa4, [23], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOB7', And(BitsEqual(0xa4, [23], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOZ7', None)
    ],
    'M3': [
        Function('GPIW7', BitsEqual(0xa0, [31], 0x1)),
        Function('ADC7', None)
    ],
    'M4': [
        Function('GPIW6', BitsEqual(0xa0, [30], 0x1)),
        Function('ADC6', None)
    ],
    'M5': [
        Function('GPIW5', BitsEqual(0xa0, [29], 0x1)),
        Function('ADC5', None)
    ],
    'N1': [
        Function('GPIX6', BitsEqual(0xa4, [6], 0x1)),
        Function('ADC14', None)
    ],
    'N18': [
        Function('ROMA15', And(BitsEqual(0xa4, [29], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOG5', And(BitsEqual(0xa4, [29], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOAA5', None)
    ],
    'N19': [
        Function('ROMA16', And(BitsEqual(0xa4, [30], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOG6', And(BitsEqual(0xa4, [30], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOAA6', None)
    ],
    'N2': [
        Function('GPIX5', BitsEqual(0xa4, [5], 0x1)),
        Function('ADC13', None)
    ],
    'N20': [
        Function('ROMA21', And(BitsEqual(0xa8, [3], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOR3', And(BitsEqual(0xa8, [3], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOAB3', None)
    ],
    'N21': [
        Function('ROMWE#', BitsEqual(0x8c, [5], 0x1)),
        Function('GPIOS5', None)
    ],
    'N22': [
        Function('ROMA20', And(BitsEqual(0xa8, [2], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOR2', And(BitsEqual(0xa8, [2], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOAB2', None)
    ],
    'N3': [
        Function('GPIX4', BitsEqual(0xa4, [4], 0x1)),
        Function('ADC12', None)
    ],
    'N4': [
        Function('GPIX3', BitsEqual(0xa4, [3], 0x1)),
        Function('ADC11', None)
    ],
    'N5': [
        Function('GPIX2', BitsEqual(0xa4, [2], 0x1)),
        Function('ADC10', None)
    ],
    'P18': [
        Function('ROMA3', And(BitsEqual(0xa4, [17], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOB1', And(BitsEqual(0xa4, [17], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOZ1', None)
    ],
    'P19': [
        Function('ROMA4', And(BitsEqual(0xa4, [18], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOB2', And(BitsEqual(0xa4, [18], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOZ2', None)
    ],
    'P20': [
        Function('ROMA5', And(BitsEqual(0xa4, [19], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOB3', And(BitsEqual(0xa4, [19], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOZ3', None)
    ],
    'P21': [
        Function('ROMA6', And(BitsEqual(0xa4, [20], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOB4', And(BitsEqual(0xa4, [20], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOZ4', None)
    ],
    'P22': [
        Function('ROMA7', And(BitsEqual(0xa4, [21], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOB5', And(BitsEqual(0xa4, [21], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOZ5', None)
    ],
    'P5': [
        Function('GPIX7', BitsEqual(0xa4, [7], 0x1)),
        Function('ADC15', None)
    ],
    'R18': [
        Function('ROMOE#', BitsEqual(0x8c, [4], 0x1)),
        Function('GPIOS4', None)
    ],
    'R22': [
        Function('ROMA2', And(BitsEqual(0xa4, [16], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOB0', And(BitsEqual(0xa4, [16], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOZ0', None)
    ],
    'T1': [
        Function('DDCDAT', BitsEqual(0x84, [15], 0x1)),
        Function('GPIOJ7', None)
    ],
    'T18': [
        Function('ROMA14', And(BitsEqual(0xa4, [28], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOG4', And(BitsEqual(0xa4, [28], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOAA4', None)
    ],
    'T19': [
        Function('ROMD5', And(BitsEqual(0x8c, [1], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOHS', And(BitsEqual(0x8c, [1], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOS1', None)
    ],
    'T2': [
        Function('DDCCLK', BitsEqual(0x84, [14], 0x1)),
        Function('GPIOJ6', None)
    ],
    'T4': [
        Function('VGAHS', BitsEqual(0x84, [12], 0x1)),
        Function('GPIOJ4', None)
    ],
    'T5': [
        Function('VPIDE', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [17], 0x1))),
        Function('NDCD1', BitsEqual(0x84, [17], 0x1)),
        Function('GPIOL1', None)
    ],
    'U1': [
        Function('NCTS1', BitsEqual(0x84, [16], 0x1)),
        Function('GPIOL0', None)
    ],
    'U18': [
        Function('FLWP#', BitsEqual(0x84, [7], 0x1)),
        Function('GPIOG7', None)
    ],
    'U19': [
        Function('ROMCS4#', BitsEqual(0x88, [27], 0x1)),
        Function('GPIOR3', None)
    ],
    'U2': [
        Function('VGAVS', BitsEqual(0x84, [13], 0x1)),
        Function('GPIOJ5', None)
    ],
    'U20': [
        Function('ROMD7', And(BitsEqual(0x8c, [3], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOCLK', And(BitsEqual(0x8c, [3], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOS3', None)
    ],
    'U21': [
        Function('ROMD4', And(BitsEqual(0x8c, [0], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPODE', And(BitsEqual(0x8c, [0], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOS0', None)
    ],
    'U3': [
        Function('VPIODD', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [18], 0x1))),
        Function('NDSR1', BitsEqual(0x84, [18], 0x1)),
        Function('GPIOL2', None)
    ],
    'U4': [
        Function('VPIVS', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [20], 0x1))),
        Function('NDTR1', BitsEqual(0x84, [20], 0x1)),
        Function('GPIOL4', None)
    ],
    'U5': [
        Function('VPIB1', And(BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x84, [23], 0x1))),
        Function('RXD1', BitsEqual(0x84, [23], 0x1)),
        Function('GPIOL7', None)
    ],
    'V1': [
        Function('VPIHS', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [19], 0x1))),
        Function('NRI1', BitsEqual(0x84, [19], 0x1)),
        Function('GPIOL3', None)
    ],
    'V2': [
        Function('VPICLK', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [21], 0x1))),
        Function('NRTS1', BitsEqual(0x84, [21], 0x1)),
        Function('GPIOL5', None)
    ],
    'V20': [
        Function('ROMCS1#', BitsEqual(0x88, [24], 0x1)),
        Function('GPIOR0', None)
    ],
    'V21': [
        Function('ROMA24', And(BitsEqual(0x88, [28], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOR6', And(BitsEqual(0x88, [28], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOR4', None)
    ],
    'V22': [
        Function('ROMD6', And(BitsEqual(0x8c, [2], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('VPOVS', And(BitsEqual(0x8c, [2], 0x1), BitsNotEqual(0x94, [1, 0], 0x0))),
        Function('GPIOS2', None)
    ],
    'V3': [
        Function('VPIB2', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [24], 0x1))),
        Function('NCTS2', BitsEqual(0x84, [24], 0x1)),
        Function('GPIOM0', None)
    ],
    'V4': [
        Function('VPIB5', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [27], 0x1))),
        Function('NRI2', BitsEqual(0x84, [27], 0x1)),
        Function('GPIOM3', None)
    ],
    'V5': [
        Function('VPIB9', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [31], 0x1))),
        Function('RXD2', BitsEqual(0x84, [31], 0x1)),
        Function('GPIOM7', None)
    ],
    'V6': [
        Function('VPIG8', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [8], 0x1))),
        Function('GPIOO0/TACH0', None)
    ],
    'V7': [
        Function('VPIR5', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [15], 0x1))),
        Function('GPIOO7/TACH7', None)
    ],
    'W1': [
        Function('VPIB0', And(BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x84, [22], 0x1))),
        Function('TXD1', BitsEqual(0x84, [22], 0x1)),
        Function('GPIOL6', None)
    ],
    'W2': [
        Function('VPIB3', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [25], 0x1))),
        Function('NDCD2', BitsEqual(0x84, [25], 0x1)),
        Function('GPIOM1', None)
    ],
    'W21': [
        Function('ROMCS2#', BitsEqual(0x88, [25], 0x1)),
        Function('GPIOR1', None)
    ],
    'W22': [
        Function('ROMA25', And(BitsEqual(0x88, [29], 0x1), BitsEqual(0x94, [1], 0x0))),
        Function('VPOR7', And(BitsEqual(0x88, [29], 0x1), BitsEqual(0x94, [1], 0x1))),
        Function('GPIOR5', None)
    ],
    'W3': [
        Function('VPIB6', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [28], 0x1))),
        Function('NDTR2', BitsEqual(0x84, [28], 0x1)),
        Function('GPIOM4', None)
    ],
    'W4': [
        Function('VPIG0', And(BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x88, [0], 0x1))),
        Function('PWM0', BitsEqual(0x88, [0], 0x1)),
        Function('GPION0', None)
    ],
    'W5': [
        Function('VPIG4', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x88, [4], 0x1))),
        Function('PWM4', BitsEqual(0x88, [4], 0x1)),
        Function('GPION4', None)
    ],
    'W6': [
        Function('VPIR2', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [12], 0x1))),
        Function('GPIOO4/TACH4', None)
    ],
    'W7': [
        Function('VPIR8', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [18], 0x1))),
        Function('GPIOP2/TACH10', None)
    ],
    'Y1': [
        Function('VPIB4', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [26], 0x1))),
        Function('NDSR2', BitsEqual(0x84, [26], 0x1)),
        Function('GPIOM2', None)
    ],
    'Y2': [
        Function('VPIB7', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x84, [29], 0x1))),
        Function('NRTS2', BitsEqual(0x84, [29], 0x1)),
        Function('GPIOM5', None)
    ],
    'Y21': [
        Function('USBCKI', BitsEqual(0x70, [23], 0x1)),
        Function('WDTRST2', BitsEqual(0x84, [5], 0x1)),
        Function('GPIOG5', None)
    ],
    'Y22': [
        Function('ROMCS3#', BitsEqual(0x88, [26], 0x1)),
        Function('GPIOR2', None)
    ],
    'Y3': [
        Function('VPIG1', And(BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x88, [1], 0x1))),
        Function('PWM1', BitsEqual(0x88, [1], 0x1)),
        Function('GPION1', None)
    ],
    'Y4': [
        Function('VPIG5', And(BitsNotEqual(0x90, [5, 4], 0x0), BitsEqual(0x88, [5], 0x1))),
        Function('PWM5', BitsEqual(0x88, [5], 0x1)),
        Function('GPION5', None)
    ],
    'Y5': [
        Function('VPIG9', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [9], 0x1))),
        Function('GPIOO1/TACH1', None)
    ],
    'Y6': [
        Function('VPIR6', And(BitsEqual(0x90, [5, 4], 0x2), BitsEqual(0x88, [16], 0x1))),
        Function('GPIOP0/TACH8', None)
    ],
}
