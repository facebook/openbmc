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


# The following table is generated using:
# python ast_gpio_parser.py data/ast2500-gpio.csv
# DO NOT MODIFY THE TABLE!!!
# Manual modification will be overridden!!!

soc_gpio_table = {
    'A10': [
        Function('SDA3', BitsEqual(0x90, [16], 0x1)),
        Function('GPIOQ1', None)
    ],
    'A11': [
        Function('SCL3', BitsEqual(0x90, [16], 0x1)),
        Function('GPIOQ0', None)
    ],
    'A12': [
        Function('SD1CMD', BitsEqual(0x90, [0], 0x1)),
        Function('SDA10', BitsEqual(0x90, [23], 0x1))
    ],
    'A13': [
        Function('SDA9', And(
            BitsEqual(0x90, [22], 0x1), BitsEqual(0x90, [6], 0x0))),
        Function('TIMER6', And(
            BitsEqual(0x80, [5], 0x1), BitsEqual(0x90, [6], 0x0))),
        Function('GPIOA5', None)
    ],
    'A14': [
        Function('SPI1MOSI', BitsNotEqual(0x70, [13, 12], 0x0)),
        Function('VBMOSI', BitsEqual(0x70, [5], 0x1)),
        Function('GPIOI6', None)
    ],
    'A15': [
        Function('SPI1MISO', BitsNotEqual(0x70, [13, 12], 0x0)),
        Function('VBMISO', BitsEqual(0x70, [5], 0x1)),
        Function('GPIOI7', None)
    ],
    'A16': [
        Function('TXD6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH6', None)
    ],
    'A17': [
        Function('UNDEFINED5', BitsEqual(0x94, [7], 0x1)),
        Function('NDTR6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH4', None)
    ],
    'A18': [
        Function('UNDEFINED1', BitsEqual(0x94, [5], 0x1)),
        Function('NCTS6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH0', None)
    ],
    'A19': [
        Function('SGPS1CK', BitsEqual(0x84, [0], 0x1)),
        Function('GPIOG0', None)
    ],
    'A2': [
        Function('GPIOU0', BitsEqual(0xa0, [8], 0x1)),
        Function('RMII2TXD0', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2TXD0', None)
    ],
    'A20': [
        Function('TXD3', BitsEqual(0x80, [22], 0x1)),
        Function('GPIE6(In)', Or(
            BitsEqual(0x8c, [15], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE6', None)
    ],
    'A21': [
        Function('NDTR4', BitsEqual(0x80, [28], 0x1)),
        Function('GPIOF4', None)
    ],
    'A3': [
        Function('GPIOU6', BitsEqual(0xa0, [14], 0x1)),
        Function('RMII1RXD0', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXD0', None)
    ],
    'A4': [
        Function('GPIOU5', BitsEqual(0xa0, [13], 0x1)),
        Function('UNDEFINED26', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXCTL', None)
    ],
    'A5': [
        Function('GPIOT3', BitsEqual(0xa0, [3], 0x1)),
        Function('RMII1TXD1', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1TXD1', None)
    ],
    'A9': [
        Function('SCL4', BitsEqual(0x90, [17], 0x1)),
        Function('GPIOQ2', None)
    ],
    'AA1': [
        Function('VPIB4', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [26], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('NDSR2', And(
            BitsEqual(0x84, [26], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOM2', None)
    ],
    'AA19': [
        Function('FWSPICS1#', And(
            BitsEqual(0x88, [24], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOR0', None)
    ],
    'AA2': [
        Function('VPIB6', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [28], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('NDTR2', And(
            BitsEqual(0x84, [28], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOM4', None)
    ],
    'AA20': [
        Function('GPIOS7', None)
    ],
    'AA22': [
        Function('SALT10', BitsEqual(0xa4, [27], 0x1)),
        Function('GPIOAA3', None)
    ],
    'AA3': [
        Function('VPIG5', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x88, [5], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('PWM5', And(
            BitsEqual(0x88, [5], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPION5', None)
    ],
    'AA4': [
        Function('VPIR4', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [14], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('GPIOO6/TACH6', None)
    ],
    'AA5': [
        Function('VPIR8', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [18], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('GPIOP2/TACH10', None)
    ],
    'AB2': [
        Function('VPIB3', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [25], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('NDCD2', And(
            BitsEqual(0x84, [25], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOM1', None)
    ],
    'AB20': [
        Function('SIOPWRGD', Or(BitsEqual(0xa4, [17], 0x1), BitsEqual(0x70, [19], 0x1))),
        Function('NORA1', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOZ1', None)
    ],
    'AB3': [
        Function('VPIR2', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [12], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('GPIOO4/TACH4', None)
    ],
    'AB4': [
        Function('UNDEFINED17', And(
            BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x88, [11], 0x1))),
        Function('GPIOO3/TACH3', None)
    ],
    'AB5': [
        Function('VPIR9', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [19], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('GPIOP3/TACH11', None)
    ],
    'B1': [
        Function('GPIOT7', BitsEqual(0xa0, [7], 0x1)),
        Function('RMII2TXEN', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2TXCTL', None)
    ],
    'B10': [
        Function('OSCCLK', BitsEqual(0x2c, [1], 0x1)),
        Function('GPIOQ6', None)
    ],
    'B11': [
        Function('SD1WP#', BitsEqual(0x90, [0], 0x1)),
        Function('SDA13', BitsEqual(0x90, [26], 0x1))
    ],
    'B12': [
        Function('SD1DAT0', BitsEqual(0x90, [0], 0x1)),
        Function('SCL11', BitsEqual(0x90, [24], 0x1))
    ],
    'B13': [
        Function('MDIO2', And(
            BitsEqual(0x90, [2], 0x1), BitsEqual(0x90, [6], 0x0))),
        Function('TIMER8', And(
            BitsEqual(0x80, [7], 0x1), BitsEqual(0x90, [6], 0x0))),
        Function('GPIOA7', None)
    ],
    'B14': [
        Function('MAC1LINK', BitsEqual(0x80, [0], 0x1)),
        Function('GPIOA0', None)
    ],
    'B15': [
        Function('SPI1CS0#', BitsNotEqual(0x70, [13, 12], 0x0)),
        Function('VBCS#', BitsEqual(0x70, [5], 0x1)),
        Function('GPIOI4', None)
    ],
    'B16': [
        Function('SYSMOSI', BitsEqual(0x70, [13], 0x1)),
        Function('GPIOI2', None)
    ],
    'B17': [
        Function('UNDEFINED6', BitsEqual(0x94, [7], 0x1)),
        Function('NRTS6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH5', None)
    ],
    'B18': [
        Function('UNDEFINED2', BitsEqual(0x94, [5], 0x1)),
        Function('NDCD6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH1', None)
    ],
    'B19': [
        Function('RXD3', BitsEqual(0x80, [23], 0x1)),
        Function('GPIE6(Out)', Or(
            BitsEqual(0x8c, [15], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE7', None)
    ],
    'B2': [
        Function('GPIOT6', BitsEqual(0xa0, [6], 0x1)),
        Function('RMII2RCLKO', And(
            BitsEqual(0x70, [7], 0x0), BitsEqual(0x48, [30], 0x1))),
        Function('RGMII2TXCK', None)
    ],
    'B20': [
        Function('NCTS3', BitsEqual(0x80, [16], 0x1)),
        Function('GPIE0(In)', Or(
            BitsEqual(0x8c, [12], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE0', None)
    ],
    'B21': [
        Function('NRI4', BitsEqual(0x80, [27], 0x1)),
        Function('GPIOF3', None)
    ],
    'B22': [
        Function('NDSR4', BitsEqual(0x80, [26], 0x1)),
        Function('GPIOF2', None)
    ],
    'B3': [
        Function('GPIOU1', BitsEqual(0xa0, [9], 0x1)),
        Function('RMII2TXD1', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2TXD1', None)
    ],
    'B4': [
        Function('GPIOU4', BitsEqual(0xa0, [12], 0x1)),
        Function('RMII1RCLKI', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXCK', None)
    ],
    'B5': [
        Function('GPIOT0', BitsEqual(0xa0, [0], 0x1)),
        Function('RMII1RCLKO', And(
            BitsEqual(0x70, [6], 0x0), BitsEqual(0x48, [29], 0x1))),
        Function('RGMII1TXCK', None)
    ],
    'B9': [
        Function('SDA4', BitsEqual(0x90, [17], 0x1)),
        Function('GPIOQ3', None)
    ],
    'C1': [
        Function('GPIOV3', BitsEqual(0xa0, [19], 0x1)),
        Function('UNDEFINED27', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXCTL', None)
    ],
    'C11': [
        Function('SD1CD#', BitsEqual(0x90, [0], 0x1)),
        Function('SCL13', BitsEqual(0x90, [26], 0x1))
    ],
    'C12': [
        Function('SD1CLK', BitsEqual(0x90, [0], 0x1)),
        Function('SCL10', BitsEqual(0x90, [23], 0x1))
    ],
    'C13': [
        Function('MDC2', And(
            BitsEqual(0x90, [2], 0x1), BitsEqual(0x90, [6], 0x0))),
        Function('TIMER7', And(
            BitsEqual(0x80, [6], 0x1), BitsEqual(0x90, [6], 0x0))),
        Function('GPIOA6', None)
    ],
    'C14': [
        Function('SCL9', And(
            BitsEqual(0x90, [22], 0x1), BitsEqual(0x90, [6], 0x0))),
        Function('TIMER5', And(
            BitsEqual(0x80, [4], 0x1), BitsEqual(0x90, [6], 0x0))),
        Function('GPIOA4', None)
    ],
    'C15': [
        Function('SPI1CK', BitsNotEqual(0x70, [13, 12], 0x0)),
        Function('VBCK', BitsEqual(0x70, [5], 0x1)),
        Function('GPIOI5', None)
    ],
    'C16': [
        Function('SYSMISO', BitsEqual(0x70, [13], 0x1)),
        Function('GPIOI3', None)
    ],
    'C17': [
        Function('UNDEFINED4', BitsEqual(0x94, [6], 0x1)),
        Function('NRI6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH3', None)
    ],
    'C18': [
        Function('SYSCS#', BitsEqual(0x70, [13], 0x1)),
        Function('GPIOI0', None)
    ],
    'C19': [
        Function('SGPS1I0', BitsEqual(0x84, [2], 0x1)),
        Function('GPIOG2', None)
    ],
    'C2': [
        Function('GPIOV2', BitsEqual(0xa0, [18], 0x1)),
        Function('RMII2RCLKI', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXCK', None)
    ],
    'C20': [
        Function('NDCD3', BitsEqual(0x80, [17], 0x1)),
        Function('GPIE0(Out)', Or(
            BitsEqual(0x8c, [12], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE1', None)
    ],
    'C21': [
        Function('SD2WP#', BitsEqual(0x90, [1], 0x1)),
        Function('GPID6(Out)', Or(
            BitsEqual(0x8c, [11], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD7', None)
    ],
    'C22': [
        Function('ESPICK', BitsEqual(0x70, [25], 0x1)),
        Function('LCLK', BitsEqual(0xac, [4], 0x1)),
        Function('GPIOAC4', None)
    ],
    'C3': [
        Function('GPIOV4', BitsEqual(0xa0, [20], 0x1)),
        Function('RMII2RXD0', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXD0', None)
    ],
    'C4': [
        Function('GPIOV1', BitsEqual(0xa0, [17], 0x1)),
        Function('RMII1RXER', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXD3', None)
    ],
    'C5': [
        Function('GPIOV0', BitsEqual(0xa0, [16], 0x1)),
        Function('RMII1CRSDV', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXD2', None)
    ],
    'D1': [
        Function('GPIOV5', BitsEqual(0xa0, [21], 0x1)),
        Function('RMII2RXD1', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXD1', None)
    ],
    'D10': [
        Function('SD1DAT2', BitsEqual(0x90, [0], 0x1)),
        Function('SCL12', BitsEqual(0x90, [25], 0x1))
    ],
    'D13': [
        Function('SPI1CS1#', BitsEqual(0x80, [15], 0x1)),
        Function('TIMER3', BitsEqual(0x80, [2], 0x1)),
        Function('GPIOA2', None)
    ],
    'D14': [
        Function('MAC2LINK', BitsEqual(0x80, [1], 0x1)),
        Function('GPIOA1', None)
    ],
    'D15': [
        Function('SGPS2I0', BitsEqual(0x94, [12], 0x1)),
        Function('SALT3', BitsEqual(0x84, [6], 0x1)),
        Function('GPIOG6', None)
    ],
    'D16': [
        Function('SGPS2LD', BitsEqual(0x94, [12], 0x1)),
        Function('SALT2', BitsEqual(0x84, [5], 0x1)),
        Function('GPIOG5', None)
    ],
    'D17': [
        Function('UNDEFINED3', BitsEqual(0x94, [6], 0x1)),
        Function('NDSR6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH2', None)
    ],
    'D18': [
        Function('RXD6', BitsEqual(0x90, [7], 0x1)),
        Function('GPIOH7', None)
    ],
    'D19': [
        Function('NRTS3', BitsEqual(0x80, [21], 0x1)),
        Function('GPIE4(Out)', Or(
            BitsEqual(0x8c, [14], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE5', None)
    ],
    'D2': [
        Function('GPIOV6', BitsEqual(0xa0, [22], 0x1)),
        Function('RMII2CRSDV', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXD2', None)
    ],
    'D20': [
        Function('SD2DAT1', BitsEqual(0x90, [1], 0x1)),
        Function('GPID2(Out)', Or(
            BitsEqual(0x8c, [9], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD3', None)
    ],
    'D21': [
        Function('SD2DAT2', BitsEqual(0x90, [1], 0x1)),
        Function('GPID4(In)', Or(
            BitsEqual(0x8c, [10], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD4', None)
    ],
    'D22': [
        Function('ESPID2', BitsEqual(0x70, [25], 0x1)),
        Function('LAD2', BitsEqual(0xac, [2], 0x1)),
        Function('GPIOAC2', None)
    ],
    'D4': [
        Function('GPIOU3', BitsEqual(0xa0, [11], 0x1)),
        Function('UNDEFINED25', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2TXD3', None)
    ],
    'D5': [
        Function('GPIOU2', BitsEqual(0xa0, [10], 0x1)),
        Function('UNDEFINED24', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2TXD2', None)
    ],
    'D6': [
        Function('GPIOU7', BitsEqual(0xa0, [15], 0x1)),
        Function('RMII1RXD1', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1RXD1', None)
    ],
    'D7': [
        Function('GPIOT5', BitsEqual(0xa0, [5], 0x1)),
        Function('UNDEFINED23', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1TXD3', None)
    ],
    'D8': [
        Function('MDC1', BitsEqual(0x88, [30], 0x1)),
        Function('GPIOR6', None)
    ],
    'D9': [
        Function('SD1DAT1', BitsEqual(0x90, [0], 0x1)),
        Function('SDA11', BitsEqual(0x90, [24], 0x1))
    ],
    'E1': [
        Function('GPIW3', BitsEqual(0xa0, [27], 0x1)),
        Function('ADC3', None)
    ],
    'E10': [
        Function('MDIO1', BitsEqual(0x88, [31], 0x1)),
        Function('GPIOR7', None)
    ],
    'E12': [
        Function('SD1DAT3', BitsEqual(0x90, [0], 0x1)),
        Function('SDA12', BitsEqual(0x90, [25], 0x1))
    ],
    'E13': [
        Function('TIMER4', BitsEqual(0x80, [3], 0x1)),
        Function('GPIOA3', None)
    ],
    'E14': [
        Function('SGPS2I1', BitsEqual(0x94, [12], 0x1)),
        Function('SALT4', BitsEqual(0x84, [7], 0x1)),
        Function('GPIOG7', None)
    ],
    'E15': [
        Function('SYSCK', BitsEqual(0x70, [13], 0x1)),
        Function('GPIOI1', None)
    ],
    'E16': [
        Function('SGPS1I1', BitsEqual(0x84, [3], 0x1)),
        Function('GPIOG3', None)
    ],
    'E17': [
        Function('SGPS2CK', BitsEqual(0x94, [12], 0x1)),
        Function('SALT1', BitsEqual(0x84, [4], 0x1)),
        Function('GPIOG4', None)
    ],
    'E18': [
        Function('NDTR3', BitsEqual(0x80, [20], 0x1)),
        Function('GPIE4(In)', Or(
            BitsEqual(0x8c, [14], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE4', None)
    ],
    'E19': [
        Function('SGPS1LD', BitsEqual(0x84, [1], 0x1)),
        Function('GPIOG1', None)
    ],
    'E2': [
        Function('GPIW2', BitsEqual(0xa0, [26], 0x1)),
        Function('ADC2', None)
    ],
    'E20': [
        Function('SD2DAT3', BitsEqual(0x90, [1], 0x1)),
        Function('GPID4(Out)', Or(
            BitsEqual(0x8c, [10], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD5', None)
    ],
    'E21': [
        Function('SD2CMD', BitsEqual(0x90, [1], 0x1)),
        Function('GPID0(Out)', Or(
            BitsEqual(0x8c, [8], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD1', None)
    ],
    'E22': [
        Function('ESPID3', BitsEqual(0x70, [25], 0x1)),
        Function('LAD3', BitsEqual(0xac, [3], 0x1)),
        Function('GPIOAC3', None)
    ],
    'E3': [
        Function('GPIW5', BitsEqual(0xa0, [29], 0x1)),
        Function('ADC5', None)
    ],
    'E6': [
        Function('GPIOV7', BitsEqual(0xa0, [23], 0x1)),
        Function('RMII2RXER', BitsEqual(0x70, [7], 0x0)),
        Function('RGMII2RXD3', None)
    ],
    'E7': [
        Function('GPIOT4', BitsEqual(0xa0, [4], 0x1)),
        Function('UNDEFINED22', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1TXD2', None)
    ],
    'E9': [
        Function('GPIOT1', BitsEqual(0xa0, [1], 0x1)),
        Function('RMII1TXEN', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1TXCTL', None)
    ],
    'F1': [
        Function('GPIX3', BitsEqual(0xa4, [3], 0x1)),
        Function('ADC11', None)
    ],
    'F17': [
        Function('NRI3', BitsEqual(0x80, [19], 0x1)),
        Function('GPIE2(Out)', Or(
            BitsEqual(0x8c, [13], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE3', None)
    ],
    'F18': [
        Function('NDSR3', BitsEqual(0x80, [18], 0x1)),
        Function('GPIE2(In)', Or(
            BitsEqual(0x8c, [13], 0x1), BitsEqual(0x70, [22], 0x1))),
        Function('GPIOE2', None)
    ],
    'F19': [
        Function('SD2CLK', BitsEqual(0x90, [1], 0x1)),
        Function('GPID0(In)', Or(
            BitsEqual(0x8c, [8], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD0', None)
    ],
    'F2': [
        Function('GPIX0', BitsEqual(0xa4, [0], 0x1)),
        Function('ADC8', None)
    ],
    'F20': [
        Function('SD2DAT0', BitsEqual(0x90, [1], 0x1)),
        Function('GPID2(In)', Or(
            BitsEqual(0x8c, [9], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD2', None)
    ],
    'F21': [
        Function('ESPICS', BitsEqual(0x70, [25], 0x1)),
        Function('LFRAME#', BitsEqual(0xac, [5], 0x1)),
        Function('GPIOAC5', None)
    ],
    'F22': [
        Function('ESPIALT', BitsEqual(0x70, [25], 0x1)),
        Function('LSIRQ#', BitsEqual(0xac, [6], 0x1)),
        Function('GPIOAC6', None)
    ],
    'F3': [
        Function('GPIW4', BitsEqual(0xa0, [28], 0x1)),
        Function('ADC4', None)
    ],
    'F4': [
        Function('GPIW0', BitsEqual(0xa0, [24], 0x1)),
        Function('ADC0', None)
    ],
    'F5': [
        Function('GPIW1', BitsEqual(0xa0, [25], 0x1)),
        Function('ADC1', None)
    ],
    'F9': [
        Function('GPIOT2', BitsEqual(0xa0, [2], 0x1)),
        Function('RMII1TXD0', BitsEqual(0x70, [6], 0x0)),
        Function('RGMII1TXD0', None)
    ],
    'G1': [
        Function('GPIX5', BitsEqual(0xa4, [5], 0x1)),
        Function('ADC13', None)
    ],
    'G17': [
        Function('TXD4', BitsEqual(0x80, [30], 0x1)),
        Function('GPIOF6', None)
    ],
    'G18': [
        Function('SD2CD#', BitsEqual(0x90, [1], 0x1)),
        Function('GPID6(In)', Or(
            BitsEqual(0x8c, [11], 0x1), BitsEqual(0x70, [21], 0x1))),
        Function('GPIOD6', None)
    ],
    'G2': [
        Function('GPIX2', BitsEqual(0xa4, [2], 0x1)),
        Function('ADC10', None)
    ],
    'G20': [
        Function('ESPID1', BitsEqual(0x70, [25], 0x1)),
        Function('LAD1', BitsEqual(0xac, [1], 0x1)),
        Function('GPIOAC1', None)
    ],
    'G21': [
        Function('ESPID0', BitsEqual(0x70, [25], 0x1)),
        Function('LAD0', BitsEqual(0xac, [0], 0x1)),
        Function('GPIOAC0', None)
    ],
    'G22': [
        Function('ESPIRST#', BitsEqual(0x70, [25], 0x1)),
        Function('LPCRST#', BitsEqual(0xac, [7], 0x1)),
        Function('GPIOAC7', None)
    ],
    'G3': [
        Function('GPIX1', BitsEqual(0xa4, [1], 0x1)),
        Function('ADC9', None)
    ],
    'G4': [
        Function('GPIW7', BitsEqual(0xa0, [31], 0x1)),
        Function('ADC7', None)
    ],
    'G5': [
        Function('GPIW6', BitsEqual(0xa0, [30], 0x1)),
        Function('ADC6', None)
    ],
    'H18': [
        Function('RXD4', BitsEqual(0x80, [31], 0x1)),
        Function('GPIOF7', None)
    ],
    'H19': [
        Function('NRTS4', BitsEqual(0x80, [29], 0x1)),
        Function('GPIOF5', None)
    ],
    'H20': [
        Function('GPIOB7', None)
    ],
    'H22': [
        Function('LPCPME#', BitsEqual(0x80, [14], 0x1)),
        Function('GPIOB6', None)
    ],
    'H3': [
        Function('GPIX6', BitsEqual(0xa4, [6], 0x1)),
        Function('ADC14', None)
    ],
    'H4': [
        Function('GPIX7', BitsEqual(0xa4, [7], 0x1)),
        Function('ADC15', None)
    ],
    'H5': [
        Function('GPIX4', BitsEqual(0xa4, [4], 0x1)),
        Function('ADC12', None)
    ],
    'J18': [
        Function('NDCD4', BitsEqual(0x80, [25], 0x1)),
        Function('GPIOF1', None)
    ],
    'J19': [
        Function('NCTS4', BitsEqual(0x80, [24], 0x1)),
        Function('GPIOF0', None)
    ],
    'J20': [
        Function('USBCKI', BitsEqual(0x70, [23], 0x1)),
        Function('GPIOB4', None)
    ],
    'K18': [
        Function('GPIOB3', None)
    ],
    'K19': [
        Function('GPIOB0', None)
    ],
    'L1': [
        Function('SCL6', BitsEqual(0x90, [19], 0x1)),
        Function('GPIOK2', None)
    ],
    'L18': [
        Function('GPIOB2', None)
    ],
    'L19': [
        Function('GPIOB1', None)
    ],
    'L2': [
        Function('SGPMLD', BitsEqual(0x84, [9], 0x1)),
        Function('GPIOJ1', None)
    ],
    'L3': [
        Function('SCL5', BitsEqual(0x90, [18], 0x1)),
        Function('GPIOK0', None)
    ],
    'L4': [
        Function('SDA5', BitsEqual(0x90, [18], 0x1)),
        Function('GPIOK1', None)
    ],
    'M18': [
        Function('SCL1', BitsEqual(0xa4, [12], 0x1)),
        Function('GPIOY4', None)
    ],
    'M19': [
        Function('SDA1', BitsEqual(0xa4, [13], 0x1)),
        Function('GPIOY5', None)
    ],
    'M20': [
        Function('SCL2', BitsEqual(0xa4, [14], 0x1)),
        Function('GPIOY6', None)
    ],
    'N1': [
        Function('SCL7', BitsEqual(0x90, [20], 0x1)),
        Function('GPIOK4', None)
    ],
    'N18': [
        Function('SALT13', BitsEqual(0xa4, [30], 0x1)),
        Function('NORD6', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOAA6', None)
    ],
    'N2': [
        Function('SDA6', BitsEqual(0x90, [19], 0x1)),
        Function('GPIOK3', None)
    ],
    'N20': [
        Function('PEWAKE#', BitsEqual(0x2c, [29], 0x1)),
        Function('GPIOQ7', None)
    ],
    'N21': [
        Function('SCL14', BitsEqual(0x90, [27], 0x1)),
        Function('GPIOQ4', None)
    ],
    'N22': [
        Function('SDA14', BitsEqual(0x90, [27], 0x1)),
        Function('GPIOQ5', None)
    ],
    'N3': [
        Function('SGPMO', BitsEqual(0x84, [10], 0x1)),
        Function('GPIOJ2', None)
    ],
    'N4': [
        Function('SGPMI', BitsEqual(0x84, [11], 0x1)),
        Function('GPIOJ3', None)
    ],
    'N5': [
        Function('VGAHS', BitsEqual(0x84, [12], 0x1)),
        Function('UNDEFINED7', BitsEqual(0x94, [8], 0x1)),
        Function('GPIOJ4', None)
    ],
    'P1': [
        Function('SDA7', BitsEqual(0x90, [20], 0x1)),
        Function('GPIOK5', None)
    ],
    'P18': [
        Function('SALT6', BitsEqual(0x8c, [3], 0x1)),
        Function('GPIOS3', None)
    ],
    'P2': [
        Function('SCL8', BitsEqual(0x90, [21], 0x1)),
        Function('GPIOK6', None)
    ],
    'P20': [
        Function('SDA2', BitsEqual(0xa4, [15], 0x1)),
        Function('GPIOY7', None)
    ],
    'P21': [
        Function('SIOONCTRL#', Or(
            BitsEqual(0xa4, [11], 0x1), BitsEqual(0x70, [19], 0x1))),
        Function('UNDEFINED31', BitsEqual(0x94, [11], 0x1)),
        Function('GPIOY3', None)
    ],
    'P22': [
        Function('SIOPWREQ#', Or(
            BitsEqual(0xa4, [10], 0x1), BitsEqual(0x70, [19], 0x1))),
        Function('UNDEFINED30', BitsEqual(0x94, [11], 0x1)),
        Function('GPIOY2', None)
    ],
    'P3': [
        Function('VPICLK', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [21], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('NRTS1', And(
            BitsEqual(0x84, [21], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOL5', None)
    ],
    'P4': [
        Function('VPIVS', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [20], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('NDTR1', And(
            BitsEqual(0x84, [20], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOL4', None)
    ],
    'P5': [
        Function('VPIB7', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [29], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('NRTS2', And(
            BitsEqual(0x84, [29], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOM5', None)
    ],
    'R1': [
        Function('SDA8', BitsEqual(0x90, [21], 0x1)),
        Function('GPIOK7', None)
    ],
    'R18': [
        Function('SALT5', BitsEqual(0x8c, [2], 0x1)),
        Function('GPIOS2', None)
    ],
    'R19': [
        Function('GPIOS4', None)
    ],
    'R2': [
        Function('SGPMCK', BitsEqual(0x84, [8], 0x1)),
        Function('GPIOJ0', None)
    ],
    'R20': [
        Function('WDTRST2', BitsEqual(0xa8, [3], 0x1)),
        Function('GPIOAB3', None)
    ],
    'R21': [
        Function('SIOS5#', Or(
            BitsEqual(0xa4, [9], 0x1), BitsEqual(0x70, [19], 0x1))),
        Function('UNDEFINED29', BitsEqual(0x94, [10], 0x1)),
        Function('GPIOY1', None)
    ],
    'R22': [
        Function('SIOS3#', Or(
            BitsEqual(0xa4, [8], 0x1), BitsEqual(0x70, [19], 0x1))),
        Function('UNDEFINED28', BitsEqual(0x94, [10], 0x1)),
        Function('GPIOY0', None)
    ],
    'R3': [
        Function('DDCCLK', BitsEqual(0x84, [14], 0x1)),
        Function('UNDEFINED9', BitsEqual(0x94, [9], 0x1)),
        Function('GPIOJ6', None)
    ],
    'R4': [
        Function('VGAVS', BitsEqual(0x84, [13], 0x1)),
        Function('UNDEFINED8', BitsEqual(0x94, [8], 0x1)),
        Function('GPIOJ5', None)
    ],
    'R5': [
        Function('VPIB8', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [30], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('TXD2', And(
            BitsEqual(0x84, [30], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOM6', None)
    ],
    'T1': [
        Function('VPIDE', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [17], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('NDCD1', And(
            BitsEqual(0x84, [17], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOL1', None)
    ],
    'T17': [
        Function('SPI2CS0#', And(
            BitsEqual(0x88, [26], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOR2', None)
    ],
    'T19': [
        Function('FWSPICS2#', And(
            BitsEqual(0x88, [25], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOR1', None)
    ],
    'T2': [
        Function('NCTS1', BitsEqual(0x84, [16], 0x1)),
        Function('GPIOL0', None)
    ],
    'T20': [
        Function('SALT12', BitsEqual(0xa4, [29], 0x1)),
        Function('NORD5', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOAA5', None)
    ],
    'T3': [
        Function('DDCDAT', BitsEqual(0x84, [15], 0x1)),
        Function('UNDEFINED10', BitsEqual(0x94, [9], 0x1)),
        Function('GPIOJ7', None)
    ],
    'T4': [
        Function('VPIG7', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [7], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('PWM7', And(
            BitsEqual(0x88, [7], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPION7', None)
    ],
    'T5': [
        Function('VPIB9', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [31], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('RXD2', And(
            BitsEqual(0x84, [31], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOM7', None)
    ],
    'U1': [
        Function('UNDEFINED11', And(
            BitsEqual(0x90, [5], 0x1), BitsEqual(0x84, [18], 0x1))),
        Function('NDSR1', BitsEqual(0x84, [18], 0x1)),
        Function('GPIOL2', None)
    ],
    'U19': [
        Function('BMCINT', BitsEqual(0x8c, [1], 0x1)),
        Function('GPIOS1', None)
    ],
    'U2': [
        Function('VPIHS', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [19], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('NRI1', And(
            BitsEqual(0x84, [19], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOL3', None)
    ],
    'U20': [
        Function('GPIOS6', None)
    ],
    'U21': [
        Function('NORA4', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOZ4', None)
    ],
    'U22': [
        Function('SALT11', BitsEqual(0xa4, [28], 0x1)),
        Function('NORD4', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOAA4', None)
    ],
    'U3': [
        Function('VPIG3', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x88, [3], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('PWM3', And(
            BitsEqual(0x88, [3], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPION3', None)
    ],
    'U4': [
        Function('VPIG9', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [9], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('GPIOO1/TACH1', None)
    ],
    'U5': [
        Function('VPIG8', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [8], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('GPIOO0/TACH0', None)
    ],
    'V1': [
        Function('UNDEFINED12', And(
            BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x84, [22], 0x1))),
        Function('TXD1', And(
            BitsEqual(0x84, [22], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOL6', None)
    ],
    'V19': [
        Function('SPI2MISO', And(
            BitsEqual(0x88, [29], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOR5', None)
    ],
    'V2': [
        Function('UNDEFINED14', And(
            BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x88, [0], 0x1))),
        Function('PWM0', And(
            BitsEqual(0x88, [0], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPION0', None)
    ],
    'V20': [
        Function('SPI2CS1#', BitsEqual(0x8c, [0], 0x1)),
        Function('GPIOS0', None)
    ],
    'V21': [
        Function('SALT8', BitsEqual(0xa4, [25], 0x1)),
        Function('NORD1', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOAA1', None)
    ],
    'V22': [
        Function('NORA6', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOZ6', None)
    ],
    'V3': [
        Function('VPIG2', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x88, [2], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('PWM2', And(
            BitsEqual(0x88, [2], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPION2', None)
    ],
    'V4': [
        Function('VPIR6', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [16], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('GPIOP0/TACH8', None)
    ],
    'V5': [
        Function('UNDEFINED16', And(
            BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x88, [10], 0x1))),
        Function('GPIOO2/TACH2', None)
    ],
    'V6': [
        Function('UNDEFINED21', And(
            BitsEqual(0x90, [28], 0x1), BitsEqual(0x88, [23], 0x1))),
        Function('GPIOP7/TACH15', None)
    ],
    'W1': [
        Function('UNDEFINED13', And(
            BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x84, [23], 0x1))),
        Function('RXD1', And(
            BitsEqual(0x84, [23], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOL7', None)
    ],
    'W19': [
        Function('SPI2MOSI', And(
            BitsEqual(0x88, [28], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOR4', None)
    ],
    'W2': [
        Function('UNDEFINED15', And(
            BitsEqual(0x90, [5, 4], 0x3), BitsEqual(0x88, [1], 0x1))),
        Function('PWM1', And(
            BitsEqual(0x88, [1], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPION1', None)
    ],
    'W20': [
        Function('GPIOS5', None)
    ],
    'W21': [
        Function('NORA7', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOZ7', None)
    ],
    'W22': [
        Function('NORA5', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOZ5', None)
    ],
    'W3': [
        Function('VPIG4', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x88, [4], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('PWM4', And(
            BitsEqual(0x88, [4], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPION4', None)
    ],
    'W4': [
        Function('VPIR5', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [15], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('GPIOO7/TACH7', None)
    ],
    'W5': [
        Function('VPIR7', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [17], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('GPIOP1/TACH9', None)
    ],
    'W6': [
        Function('UNDEFINED20', And(
            BitsEqual(0x90, [28], 0x1), BitsEqual(0x88, [22], 0x1))),
        Function('GPIOP6/TACH14', None)
    ],
    'Y1': [
        Function('VPIB2', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [24], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('NCTS2', And(
            BitsEqual(0x84, [24], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOM0', None)
    ],
    'Y19': [
        Function('SPI2CK', And(
            BitsEqual(0x88, [27], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOR3', None)
    ],
    'Y2': [
        Function('VPIB5', And(BitsEqual(0x90, [5], 0x1), And(
            BitsEqual(0x84, [27], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('NRI2', And(
            BitsEqual(0x84, [27], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPIOM3', None)
    ],
    'Y20': [
        Function('SIOPBI#', Or(
            BitsEqual(0xa4, [16], 0x1), BitsEqual(0x70, [19], 0x1))),
        Function('NORA0', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOZ0', None)
    ],
    'Y21': [
        Function('SALT7', BitsEqual(0xa4, [24], 0x1)),
        Function('NORD0', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOAA0', None)
    ],
    'Y22': [
        Function('SALT9', BitsEqual(0xa4, [26], 0x1)),
        Function('NORD2', BitsEqual(0x90, [31], 0x1)),
        Function('GPIOAA2', None)
    ],
    'Y3': [
        Function('VPIG6', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [6], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('PWM6', And(
            BitsEqual(0x88, [6], 0x1), BitsEqual(0x94, [1, 0], 0x0))),
        Function('GPION6', None)
    ],
    'Y4': [
        Function('VPIR3', And(BitsEqual(0x90, [5, 4], 0x2), And(
            BitsEqual(0x88, [13], 0x1), BitsEqual(0x94, [1, 0], 0x0)))),
        Function('GPIOO5/TACH5', None)
    ],
    'Y5': [
        Function('UNDEFINED19', And(
            BitsEqual(0x90, [28], 0x1), BitsEqual(0x88, [21], 0x1))),
        Function('GPIOP6/TACH13', None)
    ],
    'Y6': [
        Function('UNDEFINED18', And(
            BitsEqual(0x90, [28], 0x1), BitsEqual(0x88, [20], 0x1))),
        Function('GPIOP6/TACH12', None)
    ],
}
