#!/usr/bin/env python3

import sys
import time

import traceback

from modbus_update_helper import get_parser, print_perc, retry, suppress_monitoring
from pyrmd import RackmonInterface as rmd

OPERATING_MODE_WR_REG = 0x9807
OPERATING_MODE_RD_REG = 0x9800
OPERATING_MODE_AP = 0x1
OPERATING_MODE_BOOT = 0x0

IMAGE_SIZE_WR_REG = 0x9801
NOTIFY_HEX_IMAGE_SIZE_REG = 0x9803
NOTIFY_HEX_IMAGE_SIZE_MAGIC = 0xFF00

IMAGE_WR_REG = 0x9860

NOTIFY_HEX_IMAGE_WR_REG = 0x9804
NOTIFY_HEX_MAGIC = 0xFF00

HEX_TRANSMISSION_STATUS_REG = 0x9805

FW_TRANSMISSION_STATUS_REG = 0x9806
TRANSMISSION_COMPLETE = 0x1
TRANSMISSION_PENDING = 0x0

HEX_UPDATE_RESULT_REG = 0x9808
HEX_UPDATE_SUCCESS = 0x0

parser = get_parser()


def load_file(path):
    # File is already has bytes in 2 byte words
    # in big-endian order. So, read the data and
    # for a list of register values.
    with open(path, "rb") as f:
        ret = []
        while True:
            b = f.read(2)
            if len(b) != 2:
                break
            ret.append(int.from_bytes(b, byteorder="big"))
    return ret


def write_operating_mode(addr, mode):
    rmd.write(addr, OPERATING_MODE_WR_REG, mode)
    time.sleep(1.0)


def verify_operating_mode(addr, mode):
    curr_mode = rmd.read(addr, OPERATING_MODE_RD_REG)[0]
    if curr_mode != mode:
        raise ValueError("Curr mode %d != expected %d" % (curr_mode, mode))


def write_fw_img_len(addr, image):
    # image is 2 byte ints.
    img_len = len(image) * 2
    # upper first, then lower (modbus is big-endian)
    img = [(img_len >> 16) & 0xFFFF, img_len & 0xFFFF]
    rmd.write(addr, IMAGE_SIZE_WR_REG, img)
    rmd.write(addr, NOTIFY_HEX_IMAGE_SIZE_REG, NOTIFY_HEX_IMAGE_SIZE_MAGIC)


@retry(10, delay=0.2, verbose=0)
def write_transmission_complete(addr):
    status = rmd.read(addr, HEX_TRANSMISSION_STATUS_REG)[0]
    if status == 0:
        raise ValueError("Transmission not complete")


def write_data(addr, block, block_size):
    # Pad the block
    if len(block) < block_size:
        block += [0xFFFF] * (block_size - len(block))
    rmd.write(addr, IMAGE_WR_REG, block)
    rmd.write(addr, NOTIFY_HEX_IMAGE_WR_REG, NOTIFY_HEX_MAGIC)
    # Wait at least 1s to allow PLC to communicate to HEX.
    time.sleep(1.0)
    write_transmission_complete(addr)


def send_image(addr, image):
    block_size_words = 96
    num_words = len(image)
    sent_blocks = 0
    total_blocks = len(image) // block_size_words
    if num_words % block_size_words != 0:
        total_blocks += 1
    for i in range(0, num_words, block_size_words):
        write_data(addr, image[i : i + block_size_words], block_size_words)
        sent_blocks += 1
        print_perc(
            sent_blocks * 100.0 / total_blocks,
            "Sending block %d of %d..." % (sent_blocks, total_blocks),
        )


@retry(5, delay=1.0)
def check_update_result(addr):
    result = rmd.read(addr, HEX_UPDATE_RESULT_REG)[0]
    if result != HEX_UPDATE_SUCCESS:
        raise ValueError("Update failed: %d" % (result))


def verify_update(addr):
    rmd.write(addr, FW_TRANSMISSION_STATUS_REG, TRANSMISSION_COMPLETE)
    time.sleep(1.0)
    check_update_result(addr)


def update_hex(addr, filename):
    fwimg = load_file(filename)
    write_operating_mode(addr, OPERATING_MODE_BOOT)
    verify_operating_mode(addr, OPERATING_MODE_BOOT)
    write_fw_img_len(addr, fwimg)
    send_image(addr, fwimg)
    verify_update(addr)


def main():
    args = parser.parse_args()
    with suppress_monitoring():
        try:
            update_hex(args.addr, args.file)
            print("Update Successful!")
        except Exception:
            traceback.print_exc()
            sys.exit(1)


if __name__ == "__main__":
    main()
