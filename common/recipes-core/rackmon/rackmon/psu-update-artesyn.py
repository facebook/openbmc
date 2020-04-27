#!/usr/bin/env python3
"""
usage: <address> <pickled Image> [logic | primary | secondary | discharger | all] ...
./artup.by 0xb6 firmware.pkl all
"""
import argparse
import asyncio
import json
import os
import pickle
import struct
import subprocess
import sys
from contextlib import contextmanager

import pyrmd
import srec
from pyrmd import modbuscmd, read_register, transcript


def auto_int(x):
    return int(x, 0)


parser = argparse.ArgumentParser()
parser.add_argument("--addr", type=auto_int, required=True, help="PSU Modbus Address")
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


ISP_FLASH_DATA = 0x45
ISP_CTRL_CMD = 0x42
ISP_CONFIG_STATUS = 0x43
ISP_CTRL_ENTER = 0x01
ISP_CTRL_EXIT = 0x00
ISP_CTRL_RESTART = 0x03
ADP_START_UPDATE = 0x05
ADP_UPDATE_BLOCK = 0x06
ADP_FINISH_UPDATE = 0x07
PACKET_SIZE = 64


class Statusline:
    def __init__(self):
        self.msg = ""

    def update(self, msg):
        ll = len(self.msg)
        self.msg = msg
        print(msg, end="", flush=False)
        print(" " * (ll - len(msg)), end="\r", flush=True)

    def done(self, msg):
        self.update(msg)
        self.msg = ""
        print()


sl = Statusline()


def hdlc_crc(data):
    data = bytearray(data)
    crc = 0xFFFF
    for i in data:
        temp = i & 0xFF
        temp ^= crc
        temp &= 0xFF
        temp ^= temp << 4
        temp &= 0xFF
        crc = (
            (temp << 8) & 0xFFFF
            ^ ((crc >> 8) & 0x00FF)
            ^ ((temp >> 4) & 0xFFFF)
            ^ ((temp << 3) & 0xFFFF)
        )
    crc ^= 0xFFFF
    crc &= 0xFFFF
    data.append(crc & 0xFF)
    data.append(((crc >> 8) & 0xFF))
    return data


def adp_start_update(p):
    return hdlc_crc(struct.pack("BBBBB", ADP_START_UPDATE, 0x00, p, 0x01, 0x00))


def isp_enter(target):
    cmd = struct.pack("BBB", ISP_CTRL_CMD, ISP_CTRL_ENTER, target.boot_type)
    if target.adp:
        cmd += adp_start_update(target.adp)
    return cmd


def isp_data(target, data, seqbytes):
    cmd = struct.pack("BB", ISP_FLASH_DATA, len(data) + 2)
    cmd += seqbytes
    cmd += data
    return cmd


def isp_exit(target):
    cmd = struct.pack("BBB", ISP_CTRL_CMD, ISP_CTRL_EXIT, target.boot_type)
    if target.adp:
        cmd += hdlc_crc(
            struct.pack("BBBBB", ADP_FINISH_UPDATE, 0x00, target.adp, 0x01, 0x00)
        )
    return cmd


class ArtesynTarget:
    __slots__ = ["name", "startseq", "begin", "end", "boot_type", "adp"]

    def __init__(self, name, startseq, begin, end, boot_type, adp=None):
        self.name = name
        self.startseq = startseq
        # Convert wordaddrs to byteaddrs
        self.begin = begin * 2
        self.end = (end + 1) * 2
        self.boot_type = boot_type
        self.adp = adp


targets = {
    t.name: t
    for t in [
        ArtesynTarget("primary", 0x0, 0x00000, 0x05DFF, 0xAA, 0x41),
        ArtesynTarget("secondary", 0x300, 0x06000, 0x0BDFF, 0xBB, 0x42),
        ArtesynTarget("discharger", 0x600, 0x0C000, 0x11DFF, 0xDD, 0x43),
        ArtesynTarget("logic", 0x900, 0x12000, 0x183FF, 0xCC),
    ]
}


def hd(bs):
    return " ".join("{:02x}".format(b) for b in bs)


class RequestError(Exception):
    def __init__(self, *args):
        newargs = []
        for arg in args:
            if isinstance(arg, bytes):
                arg = hd(arg)
            newargs.append(arg)
        super().__init__(*newargs)


async def request(addr, modbus_cmd, *args, **kwargs):
    cmd = bytearray([addr])
    cmd.extend(modbus_cmd)
    resp = await modbuscmd(cmd, *args, **kwargs)
    # Strip address
    return resp[1:]


async def get_status(addr: int):
    bits = await request(addr, struct.pack("B", ISP_CONFIG_STATUS), expected=6)
    if bits[0] != ISP_CONFIG_STATUS:
        raise RequestError(bits[1:])
    status = int.from_bytes(bits[1:], byteorder="big")
    flags = [
        "mode",
        "startup_error",
        "checksum_error",
        "boundary_error",
        "packet_size_error",
        "pass_code_error",
    ]
    res = {}
    for i, flag in enumerate(flags):
        res[flag] = status >> i & 0x1 == 0
    res["mode"] = "ISP" if res["mode"] else "MAP"
    return res


class ArtesynError(Exception):
    def __init__(self, *args, resp=b""):
        super().__init__(self, *args)
        self.resp = resp


async def send_target(
    addr: int, image: srec.Image, target: ArtesynTarget, prev_sent: int, total_data: int
):
    data = image[target.begin : target.end]
    print("Flashing {} on {:x}".format(target.name, addr))
    n_chunks = len(data) // PACKET_SIZE
    if len(data) % PACKET_SIZE != 0:
        print("Packet size does not evenly divide image!")
        n_chunks += 1
    sent = 0
    rlen = 0
    seq = target.startseq
    for n in range(n_chunks):
        chunk = data[n * PACKET_SIZE : (n + 1) * PACKET_SIZE]
        if len(chunk) < PACKET_SIZE:
            chunk += b"\xff" * (PACKET_SIZE - len(chunk))
        if sent == 0:
            timeout = 3000
        else:
            timeout = 500
        seqbytes = struct.pack(">H", seq)
        explen = (3 + rlen) if rlen != 0 else 0
        resp = await request(
            addr, isp_data(target, chunk, seqbytes), timeout=timeout, expected=explen
        )
        rlen = len(resp)
        sent += PACKET_SIZE
        if resp[0] == 0xC5:
            # error
            raise ArtesynError("Error response: {}".format(resp), resp=resp)
        if resp[0] == 0x45:
            if resp[4] == 0x00 or resp[4] == 0x0A:
                seq += 1
            else:
                raise ArtesynError("Block sequence error: {}".format(resp), resp=resp)
        sl.update(
            "Flashing {}... {:.2%} \r".format(
                target.name, (sent + prev_sent) / total_data
            )
        )
        status_state(
            "Flashing {}... {:.2%} \r".format(
                target.name, (sent + prev_sent) / total_data
            )
        )
        status["flash_progress_percent"] = 100 * (sent + prev_sent) / total_data
    sl.done("Flashed {}".format(target.name))
    return sent


async def fw_revision(addr):
    resp = await read_register(addr, 0x38, length=4)
    return resp.decode("ascii").strip()


async def aupd(addr, image, targetnames):
    print("fw rev... ", end="")
    try:
        print(await fw_revision(addr))
    except pyrmd.ModbusTimeout:
        print("timed out.")
    entered = []
    total_data = 0
    total_sent = 0
    try:
        for name in targetnames:
            status_state("flashing target {}".format(name))
            target = targets[name]
            entered.append(name)
            enter_tries = 3
            total_data += target.end - target.begin
            while True:
                try:
                    result = await request(addr, isp_enter(target), timeout=3000)
                    break
                except pyrmd.ModbusCRCError as e:
                    # The first of these sometimes fails, always with a CRC
                    # check failure.
                    # If one succeeds, they all do, though.
                    # TODO: figure out why
                    # I suspect the microcontroller isn't quite completing the
                    # reply before it switches into ISP Mode, and response is
                    # incomplete or garbled somehow.
                    # For now though, a retry or two gets past this point
                    # and things are pretty much fine after that.
                    enter_tries -= 1
                    if enter_tries > 0:
                        print("failed to enter bootloader, retrying...")
                        continue
                    raise e
            if target.name == "logic" and result[0] & 0x80 != 0:
                raise RequestError("Error entering ISP mode", result)
            print("ISP enter: {}".format(name))
        for name in reversed(targetnames):
            target = targets[name]
            total_sent += await send_target(addr, image, target, total_sent, total_data)
    finally:
        for name in reversed(entered):
            target = targets[name]
            sl.done("Exiting update of {}".format(target.name))
            status_state("done")
            await request(addr, isp_exit(target), timeout=3000)
    await asyncio.sleep(2)
    print("fw rev... ", end="")
    try:
        print(await fw_revision(addr))
    except pyrmd.ModbusTimeout:
        print("timed out.")


@contextmanager
def suppress_monitoring():
    """
    contextmanager to pause rackmond monitoring on entry
    and resume on exit, including exits due to exception
    """
    try:
        subprocess.check_output(["rackmonctl", "pause"])
        yield
    finally:
        subprocess.check_output(["rackmonctl", "resume"])


def main():
    args = parser.parse_args()
    addr = args.addr
    global statuspath
    statuspath = args.statusfile
    with open(args.file, "rb") as f:
        image = pickle.load(f)
    targetnames = ["logic", "discharger", "secondary", "primary"]
    loop = asyncio.get_event_loop()
    with transcript(), suppress_monitoring():
        loop.run_until_complete(aupd(addr, image, targetnames))


if __name__ == "__main__":
    main()
