#!/usr/bin/env python3

import sys
import time
import traceback
from contextlib import contextmanager

from modbus_update_helper import (
    auto_int,
    get_parser,
    print_perc,
    retry,
    suppress_monitoring,
)
from pyrmd import ModbusTimeout, RackmonInterface as rmd

# Status definitions
NORMAL_OPERATION_MODE = 0x0000
ENTERED_BOOT_MODE = 0x0001
FIRMWARE_PACKET_CORRECT = 0x0006
WAIT_STATUS = 0x0018
FIRMWARE_UPGRADE_FAILED = 0x0055
FIRMWARE_UPGRADE_SUCCESS = 0x00AA

parser = get_parser()
parser.add_argument(
    "--vendor",
    type=str,
    default="panasonic",
    choices=["panasonic", "delta"],
    help="Pick vendor for device",
)
parser.add_argument("--block-size", type=auto_int, default=96, help="Block Size")

vendor_params = {
    "panasonic": {"block_size": 96, "boot_mode": 0xAA55, "block_wait": False},
    "delta": {"block_size": 64, "boot_mode": 0xA5A5, "block_wait": True},
}


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


@retry(5, delay=0.5)
def unlock_firmware(addr):
    rmd.write(addr, 0x300, 0x55AA)


@retry(5, delay=0.5)
def enter_boot_mode(addr, boot_mode):
    rmd.write(addr, 0x301, 0xAA55)
    # Allow some time for the device to erase and prepare boot mode
    time.sleep(15)
    verify_firmware_status(addr, ENTERED_BOOT_MODE)


@retry(5, delay=1.0)
def exit_boot_mode(addr):
    try:
        rmd.write(addr, 0x304, 0x55AA, 10000)
    except ModbusTimeout:
        print("Exit boot mode timed out... Checking if we are in correct status")
        verify_firmware_status(addr, NORMAL_OPERATION_MODE)


@contextmanager
def boot_mode(addr, boot_mode):
    """
    Ensure we always exit boot mode
    """
    try:
        enter_boot_mode(addr, boot_mode)
        yield
    finally:
        time.sleep(10.0)
        exit_boot_mode(addr)


def verify_firmware_status(addr, expected_status):
    # ensure 0x302 register contains expected status
    a = rmd.read(addr, 0x302)[0]
    if a != expected_status:
        raise ValueError(
            "Bad firmware state: ", int(a), " expected: ", int(expected_status)
        )


@retry(5, delay=1.0)
def write_block(addr, data, block_size):
    if len(data) < block_size:
        # TODO this is a workaround with a bad .bin file
        # which can have spurious extra bytes at the end
        # which we need to ignore.
        return
    assert len(data) == block_size
    rmd.write(addr, 0x310, data)


@retry(500, delay=0.01, verbose=0)
def wait_write_block(addr):
    verify_firmware_status(addr, FIRMWARE_PACKET_CORRECT)


def transfer_image(addr, image, block_size_words, block_wait):
    num_words = len(image)
    sent_blocks = 0
    total_blocks = num_words // block_size_words
    if num_words % block_size_words != 0:
        total_blocks += 1
    for i in range(0, num_words, block_size_words):
        write_block(addr, image[i : i + block_size_words], block_size_words)
        if block_wait:
            wait_write_block(addr)
        else:
            time.sleep(0.1)
        print_perc(
            sent_blocks * 100.0 / total_blocks,
            "Sending block %d of %d..." % (sent_blocks, total_blocks),
        )
        sent_blocks += 1
    print_perc(100.0, "Sending block %d of %d..." % (sent_blocks, total_blocks))


def verify_firmware(addr):
    time.sleep(10.0)
    rmd.write(addr, 0x303, 0x55AA, 10000)


def update_device(addr, filename, vendor):
    global vendor_params
    vendor_param = vendor_params[vendor]
    print("parsing_fw_file")
    binimg = load_file(filename)
    print("unlock firmware")
    unlock_firmware(addr)
    print("enter boot mode")
    with boot_mode(addr, vendor_param["boot_mode"]):
        print("writing data")
        transfer_image(
            addr, binimg, vendor_param["block_size"] // 2, vendor_param["block_wait"]
        )
        print("request verify firmware")
        verify_firmware(addr)
        print("Waiting for verification to complete")
        time.sleep(10.0)
        print("check firmware status")
        verify_firmware_status(addr, FIRMWARE_UPGRADE_SUCCESS)
    print("done")


def main():
    args = parser.parse_args()
    with suppress_monitoring():
        try:
            update_device(args.addr, args.file, args.vendor)
        except Exception as e:
            print("Firmware update failed %s" % str(e))
            traceback.print_exc()
            print("Waiting for reset....")
            time.sleep(30.0)
            sys.exit(1)
        print("Resetting....")
        time.sleep(30.0)
        print("Upgrade success")


if __name__ == "__main__":
    main()
