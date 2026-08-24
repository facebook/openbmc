import os
import struct
import sys
import traceback

from modbus_update_helper import print_perc

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


def get_rpu_revision(dev):
    return "TODO"


def upgrade_unlock(dev):
    req = b"\x64\x01\x19\x00\x01"
    resp = dev.raw(req, expected=8, timeout=1000)
    if resp != req:
        raise ValueError("Upgrade unlock failed")


def rpu_command(dev, cmd, data=b""):
    req = struct.pack(">BBB", 0x65, cmd, len(data)) + data
    resp = dev.raw(req, expected=6, timeout=5000)
    func, rcmd, state = struct.unpack(">BBB", resp)
    if func != 0x65 or rcmd != cmd:
        raise ValueError("RPU Command failed")
    return Status(state)


def rpu_data(dev, block, data):
    req = struct.pack(">BHH", 0x66, block, len(data)) + data
    resp = dev.raw(req, expected=7, timeout=5000)
    func, rblock, state = struct.unpack(">BHB", resp)
    if func != 0x66 or rblock != block:
        raise ValueError(f"RPU Data for block {block} failed")
    # TODO probably raise on this as well.
    return Status(state)


def file_target(dev, fname):
    rpu_command(dev, 0x02, fname.encode("utf-8")).check("target")


def file_binlen(dev, blen):
    return rpu_command(dev, 0x04, struct.pack("<I", blen)).check("binlen")


def file_bincrc(dev, bcrc):
    return rpu_command(dev, 0x06, struct.pack("<I", bcrc)).check("bincrc")


def file_blocklen(dev, blklen):
    return rpu_command(dev, 0x6F, struct.pack("<H", blklen)).check("blocklen")


def file_block_start(dev, numblks):
    return rpu_command(dev, 0x1A, struct.pack("<H", numblks)).check("blockstart")


def file_block_end(dev):
    return rpu_command(dev, 0x4D).check("blockend")


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


def send_image(dev, blocks):
    num_blocks = len(blocks)
    for block_num in range(0, num_blocks):
        rpu_data(dev, block_num, blocks[block_num]).check(f"data-block{block_num}")
        print_perc(
            block_num * 100.0 / num_blocks,
            "Sending block %d of %d..." % (block_num, num_blocks),
        )
    print_perc(
        100.0,
        "Sending block %d of %d..." % (num_blocks, num_blocks),
    )


def update_rpu(dev, image, image_name="TODO"):
    blocks, size, crc = parse_image(image)
    upgrade_unlock(dev)
    file_target(dev, image_name)
    file_binlen(dev, size)
    file_bincrc(dev, crc)
    file_blocklen(dev, BLOCK_SIZE)
    file_block_start(dev, len(blocks))
    send_image(dev, blocks)
    file_block_end(dev)


def main(dev, file):
    with dev.suppress_monitoring():
        try:
            update_rpu(dev, file, os.path.basename(file))
        except Exception:
            print("Update Failed")
            traceback.print_exc()
            sys.exit(1)
