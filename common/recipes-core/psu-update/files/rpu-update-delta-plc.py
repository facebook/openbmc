#!/usr/bin/env python3

import sys
import time
import traceback
from contextlib import contextmanager
from io import StringIO

from modbus_update_helper import bh, get_parser, print_perc, suppress_monitoring

from pyrmd import RackmonInterface as rmd


parser = get_parser()


def get_rpu_revision(addr):
    vers_raw = rmd.read(addr, 0x1BB9, 0x4, timeout=3000)
    if len(vers_raw) != 4:
        print("WARNING: Read wrong number of registers")
    vers = bytes()
    for ver in vers_raw:
        vers += ver.to_bytes(2, "little")
    return vers.decode()


@contextmanager
def rpu_stopped(addr):
    """
    Allow operations to be performed with RPU stopped.
    """
    try:
        req = addr + b"\x05\x0c\x30\x00\x00"
        resp = rmd.raw(req, expected=8)
        if resp != req:
            raise ValueError("Bad RPU Stop response: " + bh(resp))
        yield
    finally:
        req = addr + b"\x05\x0c\x30\xff\x00"
        resp = rmd.raw(req, expected=8, timeout=3000)
        if resp != req:
            raise ValueError("Bad RPU Start response: " + bh(resp))


@contextmanager
def fw_upgrade_enabled(addr):
    try:
        req = addr + b"\x64\x01\x19\x01\x01"
        resp = rmd.raw(req, expected=8)
        if resp != req:
            raise ValueError("Bad Enable FW Upgrade response: " + bh(resp))
        yield
    finally:
        req = addr + b"\x64\x01\x19\x01\x00"
        resp = rmd.raw(req, expected=8)
        if resp != req:
            raise ValueError("Bad Disable FW Upgrade response: " + bh(resp))


def syntax_check(addr):
    req = addr + b"\x05\x0c\x31\xff\x00"
    resp = rmd.raw(req, expected=8, timeout=3000)
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


def write_block(addr, block):
    rmd.write(addr, block.addr, block.data)


def write_fw(addr, fw_file):
    for idx, block in enumerate(fw_file):
        print_perc(
            100.0 * idx / len(fw_file),
            "Writing Block %d out of %d" % (idx + 1, len(fw_file)),
        )
        write_block(addr, block)
    print_perc(100.0, "Writing Block %d out of %d" % (len(fw_file), len(fw_file)))


def update_rpu(addr, filename):
    print("Current Version: %s" % (get_rpu_revision(addr)))
    addr_b = addr.to_bytes(1, "big")
    fwimg = load_fw(filename)
    with rpu_stopped(addr_b):
        with fw_upgrade_enabled(addr_b):
            write_fw(addr, fwimg)
        syntax_check(addr_b)
    time.sleep(5.0)
    print("Version After Upgrade: %s" % (get_rpu_revision(addr)))


def main():
    args = parser.parse_args()
    with suppress_monitoring():
        try:
            update_rpu(args.addr, args.file)
        except Exception:
            print("Update Failed")
            traceback.print_exc()
            sys.exit(1)


if __name__ == "__main__":
    main()
