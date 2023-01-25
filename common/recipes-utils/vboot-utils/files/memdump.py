#!/usr/bin/env python3
#
# Copyright 2022-present Facebook. All Rights Reserved.
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
import argparse
import mmap
import os
import sys
import traceback

VERSION = 1
EC_SUCCESS = 0
EC_EXCEPT = 255


def page_align_down(addr):
    return (addr >> 12) << 12


def page_align_up(addr):
    return ((addr + (1 << 12) - 1) >> 12) << 12


def dump_data(mm, base, start, end):
    n = 16
    cur = start
    while cur < end:
        n = n if cur + n < end else end - cur
        data = mm[cur : cur + n]
        print(f"{base + cur:#08x}: ", " ".join([f"{b:02x}" for b in data]))
        cur += n


def main(addr, len, binary):
    try:
        memfn = None
        map_base = page_align_down(addr)
        map_size = page_align_up(addr + len) - map_base
        data_off = addr - map_base
        memfn = os.open("/dev/mem", os.O_RDONLY | os.O_SYNC)
        with mmap.mmap(
            memfn,
            map_size,
            mmap.MAP_SHARED,
            mmap.PROT_READ,
            offset=map_base,
        ) as mm:
            if binary:
                sys.stdout.buffer.write(mm[data_off : data_off + len])
            else:
                dump_data(mm, map_base, data_off, data_off + len)
    finally:
        if memfn is not None:
            os.close(memfn)

    return EC_SUCCESS


def fillwith(addr, len, val):
    try:
        memfn = None
        map_base = page_align_down(addr)
        map_size = page_align_up(addr + len) - map_base
        data_off = addr - map_base
        memfn = os.open("/dev/mem", os.O_RDWR | os.O_SYNC)
        with mmap.mmap(
            memfn,
            map_size,
            mmap.MAP_SHARED,
            mmap.PROT_WRITE,
            offset=map_base,
        ) as mm:
            mm.seek(data_off)
            if isinstance(val, int):
                mm.write(bytearray([val] * len))
            else:
                mm.write(val.read())
    finally:
        if memfn is not None:
            os.close(memfn)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="dump the physical memory")
    parser.add_argument(
        "--version",
        action="version",
        version="%(prog)s-v{}".format(VERSION),
    )
    parser.add_argument(
        "addr",
        type=lambda x: int(x, 0),
        help="physical memory address",
    )
    parser.add_argument(
        "len",
        nargs="?",
        type=lambda x: int(x, 0),
        default=16,
        help="dump length in bytes, default 16",
    )
    parser.add_argument(
        "-b",
        "--binary",
        action="store_true",
        help="out put as raw binary can pipe feed to hexdump",
    )
    filldata = parser.add_mutually_exclusive_group()
    filldata.add_argument(
        "-w",
        "--fillwith",
        nargs="?",
        const="0xDA",
        type=lambda x: int(x, 0) % 256,
        help="fill each byte with input value default(0xDA) before dump",
    )
    filldata.add_argument(
        "-i",
        "--inputfile",
        type=argparse.FileType("rb"),
        help="fill with input file before dump",
    )
    args = parser.parse_args()

    try:
        if args.fillwith:
            fillwith(args.addr, args.len, args.fillwith)
        elif args.inputfile:
            fillwith(args.addr, args.len, args.inputfile)
            input_size = args.inputfile.tell()
            if args.binary and input_size < args.len:
                # overide dump len to input_size to facilitate verify
                # data in mem is the same as input file
                print(f"warning: dump out {input_size} bytes only", file=sys.stderr)
                args.len = input_size

        sys.exit(main(args.addr, args.len, args.binary))
    except Exception as e:
        print("Exception: %s" % (str(e)))
        traceback.print_exc()
        sys.exit(EC_EXCEPT)
