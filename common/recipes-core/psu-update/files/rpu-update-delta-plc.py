#!/usr/bin/env python3

import sys
import time
import traceback
from contextlib import contextmanager
from io import StringIO

from modbus_update_helper import (
    bh,
    decode_modbus_address,
    get_parser,
    print_perc,
    suppress_monitoring,
)

from pyrmd import RackmonInterface as rmd


parser = get_parser()
parser.add_argument(
    "--oem-block",
    action="store_true",
    default=False,
    help="Use OEM Block programming",
)


def get_rpu_revision(addr):
    return rmd.get(addr, "RPU_PLC_FW_Revision", True)


def check_rpu_size(addr):
    size_raw = rmd.read(addr, 0x13EA, 0x1, timeout=3000)
    if len(size_raw) != 1:
        print("WARNING: Read wrong number of registers")
    if size_raw[0] != 0x61A8:
        raise ValueError("Device does not support the current FW Image")


@contextmanager
def rpu_stopped(addr, unique_addr=None):
    """
    Allow operations to be performed with RPU stopped.
    """
    try:
        req = addr + b"\x05\x0c\x30\x00\x00"
        resp = rmd.raw(req, expected=8, unique_addr=unique_addr)
        if resp != req:
            raise ValueError("Bad RPU Stop response: " + bh(resp))
        yield
    finally:
        req = addr + b"\x05\x0c\x30\xff\x00"
        resp = rmd.raw(req, expected=8, timeout=3000, unique_addr=unique_addr)
        if resp != req:
            raise ValueError("Bad RPU Start response: " + bh(resp))


@contextmanager
def fw_upgrade_enabled(addr, unique_addr=None):
    try:
        req = addr + b"\x64\x01\x19\x01\x01"
        resp = rmd.raw(req, expected=8, unique_addr=unique_addr)
        if resp != req:
            raise ValueError("Bad Enable FW Upgrade response: " + bh(resp))
        yield
    finally:
        req = addr + b"\x64\x01\x19\x01\x00"
        resp = rmd.raw(req, expected=8, unique_addr=unique_addr)
        if resp != req:
            raise ValueError("Bad Disable FW Upgrade response: " + bh(resp))


def syntax_check(addr, unique_addr=None):
    req = addr + b"\x05\x0c\x31\xff\x00"
    resp = rmd.raw(req, expected=8, timeout=3000, unique_addr=unique_addr)
    if resp != req:
        raise ValueError(
            "FW syntax check failed: " + bh(resp) + " expected: " + bh(req)
        )


class Block:
    def __init__(self, addr, str_data):
        self.addr = int(addr, 16)
        self.data = []
        f = StringIO(str_data)
        while True:
            b = f.read(4)
            if len(b) != 4:
                break
            self.data.append(int(b, 16))
        if len(self.data) > 0x64:
            raise ValueError(
                "Unsupported number of words in block: %d" % (len(self.data))
            )

    def __repr__(self):
        ret = "%04x:" % (self.addr)
        for d in self.data:
            ret += " %04x" % (d)
        return ret + "\n"


def load_fw(path):
    fw = []
    with open(path) as f:
        data = f.read().splitlines()
        for line in data:
            tmp = line.split(":")
            fw.append(Block(tmp[0], tmp[1]))
    return fw


def write_block(addr, block, oem_block, unique_addr):
    if oem_block:
        line_addr = addr & 0xFF
        baddr = block.addr
        data = block.data
        bdata = b"".join([d.to_bytes(2, "big") for d in data])
        datalen = len(data)  # Number of words
        bdatalen = len(bdata)  # Number of bytes
        hdr = (
            line_addr.to_bytes(1, "big")
            + b"\x75"
            + baddr.to_bytes(2, "big")
            + datalen.to_bytes(2, "big")
            + bdatalen.to_bytes(1, "big")
        )
        cmd = hdr + bdata
        resp = rmd.raw(cmd, expected=datalen + 9, unique_addr=unique_addr)
        expected_resp = (
            line_addr.to_bytes(1, "big")
            + b"\x75"
            + baddr.to_bytes(2, "big")
            + datalen.to_bytes(2, "big")
        )
        if resp != expected_resp:
            raise ValueError("Unexpected response header: " + bh(resp))
    else:
        rmd.write(addr, block.addr, block.data, timeout=3000)


def write_fw(addr, fw_file, oem_block, unique_addr):
    for idx, block in enumerate(fw_file):
        print_perc(
            100.0 * idx / len(fw_file),
            "Writing Block %d out of %d" % (idx + 1, len(fw_file)),
        )
        write_block(addr, block, oem_block, unique_addr)
    print_perc(100.0, "Writing Block %d out of %d" % (len(fw_file), len(fw_file)))


def update_rpu(addr, filename, oem_block):
    print("Current Version: %s" % (get_rpu_revision(addr)))
    addr_b, unique_addr = decode_modbus_address(addr)
    fwimg = load_fw(filename)
    # The image requires a new protocol not supported by all devices.
    # Check if the device can in fact do this and abort early.
    if oem_block:
        check_rpu_size(addr)
    with rpu_stopped(addr_b, unique_addr):
        with fw_upgrade_enabled(addr_b, unique_addr):
            write_fw(addr, fwimg, oem_block, unique_addr)
        syntax_check(addr_b, unique_addr)
    time.sleep(8.0)
    print("Version After Upgrade: %s" % (get_rpu_revision(addr)))


def main():
    args = parser.parse_args()
    with suppress_monitoring():
        try:
            update_rpu(args.addr, args.file, args.oem_block)
        except Exception:
            print("Update Failed")
            traceback.print_exc()
            sys.exit(1)


if __name__ == "__main__":
    main()
