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

from rackmond import configure_rackmond

# flag
#  ascii   0x80XX
#  2'comp  0x4NXX
#  decimal 0x20XX
#  bitmap 1,2 0x10XX

reglist = [
    {"begin": 0x0, "length": 8, "flags": 0x8000},  # MFR_MODEL  # ascii
    {"begin": 0x10, "length": 8, "flags": 0x8000},  # MFR_DATE  # ascii
    {"begin": 0x20, "length": 8, "flags": 0x8000},  # FB Part #  # ascii
    {"begin": 0x30, "length": 4, "flags": 0x8000},  # HW Revision  # ascii
    {"begin": 0x38, "length": 4, "flags": 0x8000},  # FW Revision  # ascii
    {"begin": 0x40, "length": 16, "flags": 0x8000},  # MFR Serial #  # ascii
    {"begin": 0x60, "length": 4, "flags": 0x8000},  # Workorder #  # ascii
    {
        "begin": 0x68,  # PSU Status
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x1000 | 1,
    },
    {
        "begin": 0x69,  # Battery Status
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 0x1000 | 1,
    },
    {
        "begin": 0x6B,  # BBU Battery Mode
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 1,
    },
    {
        "begin": 0x6C,  # BBU Battery Status
        "length": 1,
        "keep": 10,  # 10-sample ring buffer
        "flags": 1,
    },
    {"begin": 0x6D, "length": 1, "keep": 10},  # BBU Cell Voltage 1
    {"begin": 0x6E, "length": 1, "keep": 10},  # BBU Cell Voltage 2
    {"begin": 0x6F, "length": 1, "keep": 10},  # BBU Cell Voltage 3
    {"begin": 0x70, "length": 1, "keep": 10},  # BBU Cell Voltage 4
    {"begin": 0x71, "length": 1, "keep": 10},  # BBU Cell Voltage 5
    {"begin": 0x72, "length": 1, "keep": 10},  # BBU Cell Voltage 6
    {"begin": 0x73, "length": 1, "keep": 10},  # BBU Cell Voltage 7
    {"begin": 0x74, "length": 1, "keep": 10},  # BBU Cell Voltage 8
    {"begin": 0x75, "length": 1, "keep": 10},  # BBU Cell Voltage 9
    {"begin": 0x76, "length": 1, "keep": 10},  # BBU Cell Voltage 10
    {"begin": 0x77, "length": 1, "keep": 10},  # BBU Cell Voltage 11
    {"begin": 0x78, "length": 1, "keep": 10},  # BBU Cell Voltage 12
    {"begin": 0x79, "length": 1, "keep": 10},  # BBU Cell Voltage 13
    {"begin": 0x7A, "length": 1, "keep": 10},  # BBU Temp 1
    {"begin": 0x7B, "length": 1, "keep": 10},  # BBU Temp 2
    {"begin": 0x7C, "length": 1, "keep": 10},  # BBU Temp 3
    {"begin": 0x7D, "length": 1, "keep": 10},  # BBU Temp 4
    {"begin": 0x7E, "length": 1, "keep": 10},  # BBU Relative State of Charge
    {"begin": 0x7F, "length": 1, "keep": 10},  # BBU Absolute State of Charge
    {
        "begin": 0x80,  # Input VAC
        "length": 1,
        "keep": 10,
        "flags": 0x4600,  # 2'comp N=6
    },
    {"begin": 0x81, "length": 1, "keep": 10},  # BBU Battery Voltage
    {
        "begin": 0x82,  # Input Current AC
        "length": 1,
        "keep": 10,
        "flags": 0x4A00,  # 2'comp N=10
    },
    {"begin": 0x83, "length": 1, "keep": 10},  # BBU Battery Current
    {
        "begin": 0x84,  # Battery Voltage
        "length": 1,
        "keep": 10,
        "flags": 0x4900,  # 2'comp N=9
    },
    {"begin": 0x85, "length": 1, "keep": 10},  # BBU Average Current
    {
        "begin": 0x86,  # Battery Current Output
        "length": 1,
        "flags": 0x4800,  # 2'comp N=8
    },
    {"begin": 0x87, "length": 1, "keep": 10},  # BBU Remaining Capacity
    {
        "begin": 0x88,  # Battery Current Input
        "length": 1,
        "flags": 0x4C00,  # 2'comp N=12
    },
    {"begin": 0x89, "length": 1, "keep": 10},  # BBU Full Charge Capacity
    {
        "begin": 0x8A,  # Output Voltage (main converter)
        "length": 1,
        "keep": 10,
        "flags": 0x4B00,  # 2'comp N=11
    },
    {"begin": 0x8B, "length": 1, "keep": 10},  # BBU Run Time to Empty
    {
        "begin": 0x8C,  # Output Current (main converter)
        "length": 1,
        "keep": 10,
        "flags": 0x4600,  # 2'comp N=6
    },
    {"begin": 0x8D, "length": 1, "keep": 10},  # BBU Average Time to Empty
    {
        "begin": 0x8E,  # IT Load Voltage Output
        "length": 1,
        "flags": 0x4900,  # 2'comp N=9
    },
    {"begin": 0x8F, "length": 1},  # BBU Charging Current
    {
        "begin": 0x90,  # IT Load Current Output
        "length": 1,
        "flags": 0x4C00,  # 2'comp N=12
    },
    {"begin": 0x91, "length": 1, "keep": 10},  # BBU Charging Voltage
    {"begin": 0x92, "length": 1, "flags": 0x4600},  # Bulk Cap Voltage  # 2'comp N=6
    {"begin": 0x93, "length": 1, "keep": 10},  # BBU Cycle Count
    {
        "begin": 0x94,  # Input Power
        "length": 1,
        "keep": 10,
        "flags": 0x4300,  # 2'comp N=3
    },
    {"begin": 0x95, "length": 1},  # BBU Design Capacity
    {
        "begin": 0x96,  # Output Power
        "length": 1,
        "keep": 10,
        "flags": 0x4300,  # 2'comp N=3
    },
    {"begin": 0x97, "length": 1},  # BBU Design Voltage
    {"begin": 0x98, "length": 1, "flags": 0x2000},  # RPM Fan 0  # Decimal
    {"begin": 0x99, "length": 1},  # BBU At Rate
    {"begin": 0x9A, "length": 1, "flags": 0x2000},  # RPM Fan 1  # Decimal
    {"begin": 0x9B, "length": 1, "keep": 10},  # BBU At Rate Time to Full
    {
        "begin": 0x9C,  # BBU At Rate Time to Empty
        "length": 1,
        "keep": 10,
        "flags": 0x2000,  # Decimal
    },
    {"begin": 0x9D, "length": 1, "keep": 10},  # BBU At Rate OK
    {"begin": 0x9E, "length": 1, "flags": 0x4700},  # Temp 0  # 2'comp N=7
    {"begin": 0x9F, "length": 1},  # BBU Temp
    {"begin": 0xA0, "length": 1, "flags": 0x4700},  # Temp 1  # 2'comp N=7
    {"begin": 0xA1, "length": 1},  # BBU Max Error
    {
        "begin": 0xD0,  # General Alarm Status Register
        "flags": 0x1000,  # Bitmap
        "length": 1,
    },
    {
        "begin": 0xD1,  # PFC Alarm Status Register
        "flags": 0x1000,  # Bitmap
        "length": 1,
    },
    {
        "begin": 0xD2,  # LLC Alarm Status Register
        "flags": 0x1000,  # Bitmap
        "length": 1,
    },
    {
        "begin": 0xD3,  # Current Feed Alarm Status Register
        "flags": 0x1000,  # Bitmap
        "length": 1,
    },
    {
        "begin": 0xD4,  # Auxiliary Alarm Status Register
        "flags": 0x1000,  # Bitmap
        "length": 1,
    },
    {
        "begin": 0xD5,  # Battery Charger Alarm Status Register
        "flags": 0x1000,  # Bitmap
        "length": 1,
    },
    {
        "begin": 0xD7,  # Temperature Alarm Status Register
        "flags": 0x1000,  # Bitmap
        "length": 1,
    },
    {
        "begin": 0xD8,  # Fan Alarm Status Register
        "flags": 0x1000,  # Bitmap
        "length": 1,
    },
    {
        "begin": 0xD9,  # Communication Alarm Status Register
        "flags": 0x1000,  # Bitmap
        "length": 1,
    },
    {"begin": 0x106, "length": 1},  # BBU Specification Info
    {"begin": 0x107, "length": 1},  # BBU Manufacturer Date
    {"begin": 0x108, "length": 1},  # BBU Serial Number
    {"begin": 0x109, "length": 2},  # BBU Device Chemistry
    {"begin": 0x10B, "length": 2},  # BBU Manufacturer Data
    {"begin": 0x10D, "length": 8},  # BBU Manufacturer Name
    {"begin": 0x115, "length": 8},  # BBU Device Name
    {"begin": 0x11D, "length": 4},  # FB Battery Status
    {"begin": 0x121, "length": 1},  # SoH results
]


def main():
    configure_rackmond(reglist, verify_configure=True)


if __name__ == "__main__":
    main()
