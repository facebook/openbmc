#!/usr/bin/env python3

import argparse
import json
import os
import os.path
import sys
import time
import traceback
from binascii import hexlify
from contextlib import ExitStack

from pyrmd import RackmonInterface as rmd
from pyrmd import ModbusTimeout


transcript_file = None

# Status definitions
NORMAL_OPERATION_MODE = 0x0000
ENTERED_BOOT_MODE = 0x0001
FIRMWARE_PACKET_CORRECT = 0x0006
WAIT_STATUS = 0x0018
FIRMWARE_UPGRADE_FAILED = 0x0055
FIRMWARE_UPGRADE_SUCCESS = 0x00AA


def auto_int(x):
    return int(x, 0)


def bh(bs):
    """bytes to hex *str*"""
    return hexlify(bs).decode("ascii")


parser = argparse.ArgumentParser()
parser.add_argument("--addr", type=auto_int, required=True, help="Modbus Address")
parser.add_argument("--block-size", type=auto_int, default=96, help="Block Size")
parser.add_argument(
    "--statusfile", default=None, help="Write status to JSON file during process"
)
parser.add_argument(
    "--rmfwfile", action="store_true", help="Delete FW file after update completes"
)
parser.add_argument(
    "--transcript",
    action="store_true",
    help="Write modbus commands and replies to modbus-transcript.log",
)
parser.add_argument("file", help="firmware file")

status = {"pid": os.getpid(), "state": "started"}

statuspath = None


def write_status():
    global status
    if statuspath is None:
        return
    tmppath = statuspath + "~"
    with open(tmppath, "w") as tfh:
        tfh.write(json.dumps(status))
    os.rename(tmppath, statuspath)


def status_state(state):
    global status
    status["state"] = state
    write_status()


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


def unlock_firmware(addr):
    rmd.write(addr, 0x300, 0x55AA)


def enter_boot_mode(addr):
    rmd.write(addr, 0x301, 0xAA55)


def verify_firmware_status(addr, expected_status):
    # ensure 0x302 register contains expected status
    a = rmd.read(addr, 0x302)[0]
    if a != expected_status:
        raise ValueError("Bad firmware state: ", int(a))


def write_block(addr, data, block_size):
    if len(data) < block_size:
        # TODO this is a workaround with a bad .bin file
        # which can have spurious extra bytes at the end
        # which we need to ignore.
        return
    assert len(data) == block_size
    maxRetry = 5
    for retry in range(0, maxRetry):
        try:
            rmd.write(addr, 0x310, data)
            break
        except ModbusTimeout as e:
            print("Retrying after timeout")
            if retry == (maxRetry - 1):
                raise e
            time.sleep(1.0)


def transfer_image(addr, image, block_size_words):
    num_words = len(image)
    sent_blocks = 0
    total_blocks = num_words // block_size_words
    if num_words % block_size_words != 0:
        total_blocks += 1
    for i in range(0, num_words, block_size_words):
        write_block(addr, image[i : i + block_size_words], block_size_words)
        if statuspath is None:
            print(
                "\r[%.2f%%] Sending block %d of %d..."
                % (sent_blocks * 100.0 / total_blocks, sent_blocks, total_blocks),
                end="",
            )
            sys.stdout.flush()
        sent_blocks += 1
    if statuspath is None:
        print("\r[100.00%%] Sending block %d of %d..." % (sent_blocks, total_blocks))
        sys.stdout.flush()


def verify_firmware(addr):
    rmd.write(addr, 0x303, 0x55AA)


def exit_boot_mode(addr):
    rmd.write(addr, 0x304, 0x55AA)


def update_device(addr, filename, block_size=96):
    status_state("pausing_monitoring")
    rmd.pause()
    status_state("parsing_fw_file")
    binimg = load_file(filename)
    status_state("unlock firmware")
    unlock_firmware(addr)
    status_state("enter boot mode")
    enter_boot_mode(addr)
    # Allow some time for the device to erase and prepare boot mode
    time.sleep(10)
    verify_firmware_status(addr, ENTERED_BOOT_MODE)
    status_state("writing data")
    transfer_image(addr, binimg, block_size // 2)

    status_state("request verify firmware")
    verify_firmware(addr)
    status_state("check firmware status")
    try:
        # XXX some older BBU FW versions do not support verify.
        # To work around this, catch any exceptions on verify
        # and exit boot mode (which activates the firmware).
        # But, raise the exception to alert the user that
        # verify failed. Revert the try/except in future once
        # we no longer see older versions.
        verify_firmware_status(addr, FIRMWARE_UPGRADE_SUCCESS)
        status_state("exit boot mode")
        exit_boot_mode(addr)
    except Exception as e:
        status_state("exit boot mode")
        exit_boot_mode(addr)
        raise e
    status_state("done")


def main():
    args = parser.parse_args()
    with ExitStack() as stack:
        global statuspath
        global transcript_file
        statuspath = args.statusfile
        if args.transcript:
            transcript_file = stack.enter_context(open("modbus-transcript.log", "w"))
        print("statusfile %s" % statuspath)
        try:
            update_device(args.addr, args.file, args.block_size)
        except Exception as e:
            print("Firmware update failed %s" % str(e))
            traceback.print_exc()
            global status
            status["exception"] = traceback.format_exc()
            status_state("failed")
            print("Waiting for reset....")
            time.sleep(30.0)
            rmd.resume()
            if args.rmfwfile:
                os.remove(args.file)
            sys.exit(1)
        print("Resetting....")
        time.sleep(30.0)
        print("Resuming monitoring...")
        rmd.resume()
        time.sleep(10.0)
        print("Upgrade success")
        if args.rmfwfile:
            os.remove(args.file)
        sys.exit(0)


if __name__ == "__main__":
    main()
