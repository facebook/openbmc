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
from pyrmd import ModbusCRCError, ModbusTimeout, RackmonInterface as rmd

# Status definitions
NORMAL_OPERATION_MODE = 0x0000
ENTERED_BOOT_MODE = 0x0001
FIRMWARE_PACKET_CORRECT = 0x0006
WAIT_STATUS = 0x0018
FIRMWARE_UPGRADE_FAILED = 0x0055
FIRMWARE_UPGRADE_SUCCESS = 0x00AA


vendor_params = {
    "panasonic": {"block_size": 96, "boot_mode": 0xAA55, "block_wait": False},
    "delta": {"block_size": 64, "boot_mode": 0xA5A5, "block_wait": True},
    "delta_power_tether": {
        "block_size": 16,
        "boot_mode": 0xAA55,
        "block_wait": True,
        "hw_workarounds": ["WRITE_BLOCK_CRC_EXPECTED"],
    },
}

parser = get_parser()
parser.add_argument(
    "--vendor",
    type=str,
    default="panasonic",
    choices=list(vendor_params.keys()),
    help="Pick vendor for device",
)
parser.add_argument("--block-size", type=auto_int, default=None, help="Block Size")


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
    rmd.write(addr, 0x300, 0x55AA, timeout=1000)


@retry(5, delay=0.5)
def enter_boot_mode(addr, boot_mode):
    print("Entering Boot Mode...")
    rmd.write(addr, 0x301, 0xAA55, timeout=5000)
    # Allow some time for the device to erase and prepare boot mode
    time.sleep(15)
    verify_firmware_status(addr, ENTERED_BOOT_MODE)


@retry(5, delay=1.0)
def exit_boot_mode(addr):
    print("Exiting Boot Mode...")
    try:
        rmd.write(addr, 0x304, 0x55AA, timeout=10000)
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
        time.sleep(10.0)
        verify_firmware_status(addr, NORMAL_OPERATION_MODE)


def verify_firmware_status(addr, expected_status):
    # ensure 0x302 register contains expected status
    a = rmd.read(addr, 0x302, timeout=1000)[0]
    if a != expected_status:
        raise ValueError(
            "Bad firmware state: ", int(a), " expected: ", int(expected_status)
        )


@retry(5, delay=1.0)
def write_block(addr, data, block_size, workarounds):
    if len(data) < block_size:
        # TODO this is a workaround with a bad .bin file
        # which can have spurious extra bytes at the end
        # which we need to ignore.
        return
    assert len(data) == block_size
    if "WRITE_BLOCK_CRC_EXPECTED" not in workarounds:
        rmd.write(addr, 0x310, data, timeout=1000)
        return
    try:
        rmd.write(addr, 0x310, data, timeout=1000)
    except ModbusCRCError:
        # Ignore CRC Error to support early boot-loaders
        # which respond with incorrect CRC16 code.
        # TODO remove when no longer required.
        if "PRINTED" not in workarounds:
            workarounds.append("PRINTED")
            print("WARNING: CRCError suppressed")


@retry(500, delay=0.01, verbose=0)
def wait_write_block(addr):
    verify_firmware_status(addr, FIRMWARE_PACKET_CORRECT)


def transfer_image(addr, image, block_size_words, block_wait, workarounds):
    num_words = len(image)
    sent_blocks = 0
    total_blocks = num_words // block_size_words
    if num_words % block_size_words != 0:
        total_blocks += 1
    for i in range(0, num_words, block_size_words):
        write_block(
            addr, image[i : i + block_size_words], block_size_words, workarounds
        )
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


def update_device(addr, filename, vendor_param):
    print("Parsing Firmware...")
    binimg = load_file(filename)
    print("Unlock Engineering Mode")
    unlock_firmware(addr)
    with boot_mode(addr, vendor_param["boot_mode"]):
        print("Transferring image")
        time.sleep(5.0)
        transfer_image(
            addr,
            binimg,
            vendor_param["block_size"] // 2,
            vendor_param["block_wait"],
            vendor_param.get("hw_workarounds", []),
        )
        print("Request Verify Firmware")
        verify_firmware(addr)
        print("Waiting for verification to complete")
        time.sleep(10.0)
        print("check firmware status")
        verify_firmware_status(addr, FIRMWARE_UPGRADE_SUCCESS)
    print("done")


def main():
    global vendor_params
    args = parser.parse_args()
    params = vendor_params[args.vendor]
    if args.block_size is not None:
        params["block_size"] = args.block_size
    print("Upgrade Parameters: ", params)
    with suppress_monitoring():
        try:
            update_device(args.addr, args.file, params)
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
