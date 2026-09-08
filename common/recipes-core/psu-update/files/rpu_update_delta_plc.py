import sys
import time
import traceback
from contextlib import contextmanager
from io import StringIO

from modbus_update_helper import bh, print_perc

# Tuple containing the register and length containing the PLC version
RPU_PLC_FW_Revision = (6632, 4)

def get_rpu_revision(dev):
    reg, rlen = RPU_PLC_FW_Revision
    return dev.read_str(reg, rlen)


def check_rpu_size(dev):
    size_raw = dev.read(0x13EA, 0x1, timeout=3000)
    if len(size_raw) != 1:
        print("WARNING: Read wrong number of registers")
    if size_raw[0] != 0x61A8:
        raise ValueError("Device does not support the current FW Image")


@contextmanager
def rpu_stopped(dev):
    """
    Allow operations to be performed with RPU stopped.
    """
    try:
        req = b"\x05\x0c\x30\x00\x00"
        resp = dev.raw(req, expected=8)
        if resp != req:
            raise ValueError("Bad RPU Stop response: " + bh(resp))
        yield
    finally:
        req = b"\x05\x0c\x30\xff\x00"
        resp = dev.raw(req, expected=8, timeout=3000)
        if resp != req:
            raise ValueError("Bad RPU Start response: " + bh(resp))


@contextmanager
def fw_upgrade_enabled(dev):
    try:
        req = b"\x64\x01\x19\x01\x01"
        resp = dev.raw(req, expected=8)
        if resp != req:
            raise ValueError("Bad Enable FW Upgrade response: " + bh(resp))
        yield
    finally:
        req = b"\x64\x01\x19\x01\x00"
        resp = dev.raw(req, expected=8)
        if resp != req:
            raise ValueError("Bad Disable FW Upgrade response: " + bh(resp))


def syntax_check(dev):
    req = b"\x05\x0c\x31\xff\x00"
    resp = dev.raw(req, expected=8, timeout=3000)
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


def write_block(dev, block, oem_block):
    if oem_block:
        baddr = block.addr
        data = block.data
        bdata = b"".join([d.to_bytes(2, "big") for d in data])
        datalen = len(data)  # Number of words
        bdatalen = len(bdata)  # Number of bytes
        hdr = (
            b"\x75"
            + baddr.to_bytes(2, "big")
            + datalen.to_bytes(2, "big")
            + bdatalen.to_bytes(1, "big")
        )
        cmd = hdr + bdata
        resp = dev.raw(cmd, expected=datalen + 9)
        expected_resp = b"\x75" + baddr.to_bytes(2, "big") + datalen.to_bytes(2, "big")
        if resp != expected_resp:
            raise ValueError("Unexpected response header: " + bh(resp))
    else:
        dev.write(block.addr, block.data, timeout=3000)


def write_fw(dev, fw_file, oem_block):
    for idx, block in enumerate(fw_file):
        print_perc(
            100.0 * idx / len(fw_file),
            "Writing Block %d out of %d" % (idx + 1, len(fw_file)),
        )
        write_block(dev, block, oem_block)
    print_perc(100.0, "Writing Block %d out of %d" % (len(fw_file), len(fw_file)))


def update_rpu(dev, filename, oem_block):
    print("Current Version: %s" % (get_rpu_revision(dev)))
    fwimg = load_fw(filename)
    # The image requires a new protocol not supported by all devices.
    # Check if the device can in fact do this and abort early.
    if oem_block:
        check_rpu_size(dev)
    with rpu_stopped(dev):
        with fw_upgrade_enabled(dev):
            write_fw(dev, fwimg, oem_block)
        syntax_check(dev)
    time.sleep(8.0)
    print("Version After Upgrade: %s" % (get_rpu_revision(dev)))


def main(dev, file, oem_block):
    try:
        update_rpu(dev, file, oem_block)
    except Exception:
        print("Update Failed")
        traceback.print_exc()
        sys.exit(1)
