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
# This script is to be used in OpenBmc for reading information of
# power supplies from i2c bus.
# Version 1.0

import argparse
import sys
from ctypes import (
    POINTER,
    Structure,
    c_uint8,
    c_uint16,
    c_uint32,
    cast,
    create_string_buffer,
    pointer,
)
from fcntl import ioctl


I2C_M_RD = 0x0001
I2C_RDWR = 0x0707


class i2c_msg(Structure):
    _fields_ = [
        ("addr", c_uint16),
        ("flags", c_uint16),
        ("len", c_uint16),
        ("buf", POINTER(c_uint8)),
    ]


class i2c_rdwr_ioctl_data(Structure):
    _fields_ = [("msgs", POINTER(i2c_msg)), ("nmsgs", c_uint32)]


def _makeI2cRdwrRequest(addr, reg, size, data, read):
    msg_data_type = i2c_msg * 2
    msg_data = msg_data_type()
    msg_data[0].addr = addr
    msg_data[0].flags = 0
    msg_data[0].len = 1
    msg_data[0].buf = reg
    msg_data[1].addr = addr
    msg_data[1].flags = I2C_M_RD if read else 0
    msg_data[1].len = size
    msg_data[1].buf = data
    request = i2c_rdwr_ioctl_data()
    request.msgs = msg_data
    request.nmsgs = 2
    return request


class I2cMsg(object):
    def __init__(self, bus):
        self.bus = bus
        self.device = None

    def __str__(self):
        return "%s(addr=%s, device=%s)" % (
            self.__class__.__name__,
            self.bus,
            self.device,
        )

    def __enter__(self):
        if self.device is None:
            self.device = open("/dev/i2c-%d" % self.bus, "r+b", buffering=0)
        return self

    def __exit__(self, *args):
        if self.device:
            self.device.close()
            self.device = None

    def getI2cBlock(self, devAddr, command, length):
        result = create_string_buffer(length)
        reg = c_uint8(command)
        request = _makeI2cRdwrRequest(
            devAddr, pointer(reg), length, cast(result, POINTER(c_uint8)), 1
        )
        ioctl(self.device.fileno(), I2C_RDWR, request)
        return result.raw


def decodeHex(data):
    return "0x" + "".join(["{:02x}".format(v) for v in data])


def decodeAscii(data):
    return "".join([chr(v) if 31 < v < 128 else "_" for v in data])


def decodeUnknown(data):
    return decodeHex(data) + " (ascii: " + decodeAscii(data) + ")"


def readBlock(i2cBus, chipAddr, regAddr, readLen, name=None):
    with I2cMsg(i2cBus) as w:
        if readLen is None:
            payloadLen = w.getI2cBlock(chipAddr, regAddr, 1)[0]
            readLen = 1 + payloadLen
        else:
            payloadLen = readLen

        block = w.getI2cBlock(chipAddr, regAddr, readLen)
        return block[-payloadLen:]


class PowerSupply(object):
    registers = []
    registersExtra = []

    def __init__(self, bus, chip):
        self.i2cBus = bus
        self.chipAddr = chip

    def showTech(self, detail=False):
        registers = self.registers[:]
        if detail:
            registers.extend(self.registersExtra)
        for name, addr, readLen, decodeFunc in registers:
            try:
                data = readBlock(self.i2cBus, self.chipAddr, addr, readLen, name=name)
                value = decodeFunc(data)
            except Exception:
                value = "Error"
            print(name + ":", value)


class DPS1900(PowerSupply):
    registers = [
        ("MFR_ID", 0x99, None, decodeAscii),
        ("MFR_MODEL", 0x9A, None, decodeAscii),
        ("MFR_REVISION", 0x9B, None, decodeAscii),
        ("MFR_LOCATION", 0x9C, None, decodeAscii),
        ("MFR_DATE", 0x9D, None, decodeAscii),
        ("MFR_SERIAL", 0x9E, None, decodeAscii),
        ("FW_VERSION", 0xD5, 8, decodeAscii),
    ]
    registersExtra = [
        ("PRIMARY_REVISION", 0xE0, 8, decodeAscii),
        ("SECOND_REVISION", 0xE1, 8, decodeAscii),
        ("STATUS_BYTE", 0x78, 1, decodeHex),
        ("STATUS_WORD", 0x79, 2, decodeHex),
        ("STATUS_VOUT", 0x7A, 1, decodeHex),
        ("STATUS_IOUT", 0x7B, 1, decodeHex),
        ("STATUS_INPUT", 0x7C, 1, decodeHex),
        ("STATUS_TEMP", 0x7D, 1, decodeHex),
        ("STATUS_CML", 0x7E, 1, decodeHex),
        ("STATUS_OTHER", 0x7F, 1, decodeHex),
        ("STATUS_MFR", 0x80, 1, decodeHex),
        ("STATUS_FAN_1_2", 0x81, 1, decodeHex),
        ("READ_VIN", 0x88, 2, decodeHex),
        ("READ_IIN", 0x89, 2, decodeHex),
        ("READ_VOUT", 0x8B, 2, decodeHex),
        ("READ_IOUT", 0x8C, 2, decodeHex),
        ("READ_TEMP1", 0x8D, 2, decodeHex),
        ("READ_TEMP2", 0x8E, 2, decodeHex),
        ("READ_TEMP3", 0x8F, 2, decodeHex),
        ("READ_FAN_SPEED_1", 0x90, 2, decodeHex),
        ("READ_POUT", 0x96, 2, decodeHex),
        ("READ_PIN", 0x97, 2, decodeHex),
    ]


PROFILES = {"dps1900": DPS1900}


def parseArgs():
    parser = argparse.ArgumentParser(description="Get power supply info from i2c bus")
    parser.add_argument("i2cBus", type=int, help="i2c bus, e.g., 5")
    parser.add_argument("chipAddr", help="chip address (hex), e.g. 0x58")
    group1 = parser.add_mutually_exclusive_group(required=True)
    group1.add_argument("-c", "--chipModel", help="chip model, e.g., dps1900")
    group1.add_argument("-r", "--regAddr", help="register address (hex), e.g., 0x10")
    parser.add_argument("-l", "--byteLength", default=1, type=int, help="bytes to read")
    parser.add_argument("-d", "--detail", action="store_true", help="show detail")
    return parser.parse_args()


def main():
    args = parseArgs()
    chipAddr = int(args.chipAddr, 16)
    if args.chipModel:
        profile = PROFILES.get(args.chipModel)
        if profile is None:
            print("Profile for", args.chipModel, "is not found")
            sys.exit(1)
        profile(args.i2cBus, chipAddr).showTech(args.detail)
    else:
        regAddr = int(args.regAddr, 16)
        data = readBlock(args.i2cBus, chipAddr, regAddr, args.byteLength)
        print(decodeUnknown(data))


if __name__ == "__main__":
    main()
