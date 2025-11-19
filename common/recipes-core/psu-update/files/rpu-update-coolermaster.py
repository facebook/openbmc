#!/usr/bin/env python3

import os
import struct
import sys
import traceback

from modbus_update_helper import (
    decode_modbus_address,
    get_parser,
    print_perc,
    suppress_monitoring,
)

from pyrmd import RackmonInterface as rmd

BLOCK_SIZE = 192


class Status:
    status_map = {
        "ACK": 0xAC,
        "NACK_ERR_TCRC": 0xDF,
        "ERR_SYSTEM": 0xA1,
        "ERR_UNKNOWDATA": 0xB2,
        "ERR_UNKNOWHEAD": 0xC3,
        "ERR_OUTOFLEN": 0xD4,
        "ERR_ENDLEN": 0xE5,
        "ERR_ENDCRC": 0xF6,
        "ERR_BUSY": 0xBB,
    }

    def __init__(self, val):
        self.val = val

    def __str__(self):
        for k, v in self.status_map.items():
            if v == self.val:
                return k
        return "%02x" % (self.val)

    def check(self, cmd):
        if self.val == 0xAC:
            return
        raise ValueError("Failed command %s : %s" % (cmd, str(self)))


parser = get_parser()


def get_rpu_revision(addr):
    return "TODO"


def upgrade_unlock(addr):
    laddr, uaddr = decode_modbus_address(addr, True)
    req = laddr + b"\x64\x01\x19\x00\x01"
    resp = rmd.raw(req, expected=8, unique_addr=uaddr, timeout=1000)
    if resp != req:
        raise ValueError("Upgrade unlock failed")


def rpu_command(addr, cmd, data=b""):
    laddr, uaddr = decode_modbus_address(addr, True)
    req = laddr + struct.pack(">BBB", 0x65, cmd, len(data)) + data
    resp = rmd.raw(req, expected=6, unique_addr=uaddr, timeout=5000)
    raddr, func, rcmd, state = struct.unpack(">BBBB", resp)
    if raddr != int.from_bytes(laddr, byteorder="big") or func != 0x65 or rcmd != cmd:
        raise ValueError("RPU Command failed")
    return Status(state)


def rpu_data(addr, block, data):
    laddr, uaddr = decode_modbus_address(addr, True)
    req = laddr + struct.pack(">BHH", 0x66, block, len(data)) + data
    resp = rmd.raw(req, expected=7, unique_addr=uaddr, timeout=5000)
    raddr, func, rblock, state = struct.unpack(">BBHB", resp)
    if (
        raddr != int.from_bytes(laddr, byteorder="big")
        or func != 0x66
        or rblock != block
    ):
        raise ValueError(f"RPU Data for block {block} failed")
    # TODO probably raise on this as well.
    return Status(state)


def file_target(addr, fname):
    rpu_command(addr, 0x02, fname.encode("utf-8")).check("target")


def file_binlen(addr, blen):
    return rpu_command(addr, 0x04, struct.pack("<I", blen)).check("binlen")


def file_bincrc(addr, bcrc):
    return rpu_command(addr, 0x06, struct.pack("<I", bcrc)).check("bincrc")


def file_blocklen(addr, blklen):
    return rpu_command(addr, 0x6F, struct.pack("<H", blklen)).check("blocklen")


def file_block_start(addr, numblks):
    return rpu_command(addr, 0x1A, struct.pack("<H", numblks)).check("blockstart")


def file_block_end(addr):
    return rpu_command(addr, 0x4D).check("blockend")


def crc32mpeg2(buf):
    crc = 0xFFFFFFFF
    for val in buf:
        val = int(val)
        crc = (crc ^ (val << 24)) & 0xFFFFFFFF
        for _ in range(8):
            crc = (
                crc << 1 if (crc & 0x80000000) == 0 else (crc << 1) ^ 0x4C11DB7
            ) & 0xFFFFFFFF
    return crc


def parse_image(ipath):
    blocks = []
    size = 0
    crc = 0
    with open(ipath, "rb") as f:
        data = f.read()
        size = len(data)
        crc = crc32mpeg2(data)
        for start in range(0, size, BLOCK_SIZE):
            rem = len(data) - start
            if rem <= BLOCK_SIZE:
                block = data[start:]
            else:
                block = data[start : start + BLOCK_SIZE]
            blocks.append(block)
    return blocks, size, crc


def send_image(addr, blocks):
    num_blocks = len(blocks)
    for block_num in range(0, num_blocks):
        rpu_data(addr, block_num, blocks[block_num]).check(f"data-block{block_num}")
        print_perc(
            block_num * 100.0 / num_blocks,
            "Sending block %d of %d..." % (block_num, num_blocks),
        )
    print_perc(
        100.0,
        "Sending block %d of %d..." % (num_blocks, num_blocks),
    )


def update_rpu(addr, image, image_name="TODO"):
    blocks, size, crc = parse_image(image)
    upgrade_unlock(addr)
    file_target(addr, image_name)
    file_binlen(addr, size)
    file_bincrc(addr, crc)
    file_blocklen(addr, BLOCK_SIZE)
    file_block_start(addr, len(blocks))
    send_image(addr, blocks)
    file_block_end(addr)


def main():
    args = parser.parse_args()
    with suppress_monitoring():
        try:
            update_rpu(args.addr, args.file, os.path.basename(args.file))
        except Exception:
            print("Update Failed")
            traceback.print_exc()
            sys.exit(1)


if __name__ == "__main__":
    main()
