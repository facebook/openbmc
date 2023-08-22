#!/usr/bin/env python3

import struct
import sys
import time
import traceback

from modbus_update_helper import get_parser, print_perc, suppress_monitoring

from pyrmd import ModbusException, RackmonInterface as rmd


ISP_CTRL_CMD_ENTER = 0x1
ISP_CTRL_CMD_EXIT = 0x0

BLOCK_WRITE_SUCCESS = 0x0
BLOCK_WRITE_BLOCK_WRITTEN = 0x1
BLOCK_WRITE_INVALID_RANGE = 0x2


parser = get_parser()


class StatusRegister:
    _fields_ = [
        # Byte 0
        "FULL_IMAGE_RECEIVED",
        "FULL_IMAGE_RECV_PENDING",
        "RESERVED",
        "FULL_IMAGE_RECEIVED_CORRUPT",
        "RESERVED",
        "RESERVED",
        "RESERVED",
        "RESERVED",
        # Byte 1
        "PRIMARY_DSP_CHECKSUM_FAILED",
        "SECONDARY_DSP_CHECKSUM_FAILED",
        "LOGIC_DSP_CHECKSUM_FAILED",
        "PRIMARY_DSP_CHECKSUM_PASSED",
        "SECONDARY_DSP_CHECKSUM_PASSED",
        "LOGIC_DSP_CHECKSUM_PASSED",
        "PRIMARY_DSP_UPDATE_FAILED",
        "SECONDARY_DSP_UPDATE_FAILED",
    ]

    def __init__(self, val):
        self.val = val

    def __getitem__(self, name):
        return (self.val & (1 << self._fields_.index(name))) != 0

    def __str__(self):
        return str(
            [
                (name, (self.val & (1 << idx)) != 0)
                for idx, name in enumerate(self._fields_)
            ]
        )

    def getSet(self):
        return {
            field
            for bit, field in enumerate(self._fields_)
            if field != "RESERVED" and ((1 << bit) & self.val) != 0
        }


class BadAEIResponse(ModbusException):
    ...


def load_file(path):
    with open(path, "rb") as f:
        return f.read()
    raise ValueError("Failed Loading image")


def isp_get_status(addr):
    req = struct.pack(">BBB", addr, 0x43, 2)
    resp = rmd.raw(req, expected=7)
    raddr, rfunc, bcount, status = struct.unpack(">BBBH", resp)
    if raddr != addr or rfunc != 0x43:
        raise BadAEIResponse()
    return StatusRegister(status)


def isp_enter(addr):
    req = struct.pack(">BBB", addr, 0x42, ISP_CTRL_CMD_ENTER)
    resp = rmd.raw(req, expected=7)
    raddr, rfun, rcmd, _ = struct.unpack(">BBBB", resp)
    if raddr != addr or rfun != 0x42 or rcmd != ISP_CTRL_CMD_ENTER:
        raise BadAEIResponse()
    st = isp_get_status(addr)
    if not st["FULL_IMAGE_RECV_PENDING"]:
        print(
            "PSU not ready to receive image after entering ISP mode. Status:", str(st)
        )
        raise BadAEIResponse()


def isp_exit(addr):
    req = struct.pack(">BBB", addr, 0x42, ISP_CTRL_CMD_EXIT)
    resp = rmd.raw(req, expected=7)
    raddr, rfun, rcmd, status, _ = struct.unpack(">BBBBB", resp)
    if raddr != addr or rfun != 0x42 or rcmd != ISP_CTRL_CMD_EXIT:
        raise BadAEIResponse()
    # 0 - success, 1 - failure
    if status != 0:
        print(f"Status from exit returned {status}")
        raise BadAEIResponse()


def isp_flash_block(addr, block_no, block):
    bsize = 64
    if len(block) != bsize:
        raise ValueError(f"Unexpected block of size {len(block)}")
    req = struct.pack(">BBBH", addr, 0x45, bsize + 2, block_no) + block
    resp = rmd.raw(req, expected=8)
    raddr, rfunc, rlen, rblock, rcode = struct.unpack(">BBBHB", resp)
    if raddr != addr or rfunc != 0x45 or rlen != 0x3 or rblock != block_no:
        print("Bad Block write response:", resp)
        raise BadAEIResponse()
    if rcode == BLOCK_WRITE_BLOCK_WRITTEN:
        print(f"WARNING: Skipping block {block_no} since its already written")
    elif rcode == BLOCK_WRITE_INVALID_RANGE:
        print(f"ERROR: Invalid range: {block_no}")
        raise BadAEIResponse()
    elif rcode != BLOCK_WRITE_SUCCESS:
        print(f"ERROR: Unknown return code {rcode} received")
        raise BadAEIResponse()


def transfer_image(addr, image):
    block_size = 64
    sent_blocks = 0
    total_blocks = len(image) // block_size
    if len(image) % block_size != 0:
        print("Unsupported file of size {len(image)} has partial block size at the end")
        raise BadAEIResponse()
    for i in range(0, len(image), block_size):
        isp_flash_block(addr, sent_blocks, image[i : i + block_size])
        sent_blocks += 1
        print_perc(
            sent_blocks * 100.0 / total_blocks,
            "Sending block# %d of %d..." % (sent_blocks, total_blocks - 1),
        )
    last_block = total_blocks - 1
    print_perc(100.0, "Sending block %d of %d..." % (last_block, last_block))


def wait_update_complete(addr):
    last_set = set()
    error_fields = {
        "FULL_IMAGE_RECEIVED_CORRUPT",
        "PRIMARY_DSP_CHECKSUM_FAILED",
        "SECONDARY_DSP_CHECKSUM_FAILED",
        "LOGIC_DSP_CHECKSUM_FAILED",
        "PRIMARY_DSP_UPDATE_FAILED",
        "SECONDARY_DSP_UPDATE_FAILED",
    }
    max_time = 360
    for _ in range(max_time):
        try:
            status = isp_get_status(addr)
            set_fields = status.getSet()
            errors = set_fields.intersection(error_fields)
            if errors:
                raise ValueError("ERROR: " + " ".join(errors))
            if last_set != set_fields:
                if status.val == 0:
                    print("PSU Rebooted!")
                    break
                added = set_fields - last_set
                last_set = set_fields
                print("ALERT:", " ".join(added))
        except Exception:
            # Ignore timeouts and other issues as the thing resets
            pass
        time.sleep(1)
    else:
        raise ValueError("Timed out waiting for PSU to reset")


def update_device(addr, filename):
    print("Parsing Firmware")
    binimg = load_file(filename)
    print("Enter ISP Mode")
    isp_enter(addr)
    time.sleep(10.0)  # Wait for PSU to erase flash.
    print("Transfer Image")
    transfer_image(addr, binimg)
    print("Check Status")
    st = isp_get_status(addr)
    if not st["FULL_IMAGE_RECEIVED"]:
        print("PSU Did not receive the full image. Status:", str(st))
        raise BadAEIResponse()
    print("Exit ISP Mode")
    isp_exit(addr)
    print("Wait update to complete. Sleeping for ~6min")
    wait_update_complete(addr)
    print("done")


def main():
    args = parser.parse_args()
    with suppress_monitoring():
        try:
            update_device(args.addr, args.file)
        except Exception as e:
            print("Firmware update failed %s" % str(e))
            traceback.print_exc()
            sys.exit(1)
    print("Upgrade success")


if __name__ == "__main__":
    main()
