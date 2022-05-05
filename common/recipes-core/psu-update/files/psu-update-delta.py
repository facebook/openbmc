#!/usr/bin/env python3

import argparse
import json
import os
import os.path
import struct
import sys
import time
import traceback
from binascii import hexlify
from contextlib import ExitStack

import hexfile
import pyrmd


transcript_file = None


def auto_int(x):
    return int(x, 0)


def bh(bs):
    """bytes to hex *str*"""
    return hexlify(bs).decode("ascii")


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


class BadMEIResponse(pyrmd.ModbusException):
    ...


def tprint(s):
    if transcript_file:
        print(s, file=transcript_file)


def mei_command(addr, func_code, mei_type=0x64, data=None, timeout=0):
    i_data = data
    if i_data is None:
        i_data = b"\xFF" * 7
    if len(i_data) < 7:
        i_data = i_data + (b"\xFF" * (7 - len(i_data)))
    assert len(i_data) == 7
    command = struct.pack("BBBB", addr, 0x2B, mei_type, func_code) + i_data
    return pyrmd.modbuscmd_sync(command, expected=13, timeout=timeout)


def enter_bootloader(addr):
    try:
        print("Entering bootloader...")
        mei_command(addr, 0xFB, timeout=4000)
    except pyrmd.ModbusTimeout:
        print("Enter bootloader timed out (expected.)")
        pass


def mei_expect(response, addr, data_pfx, error, success_mei_type=0x71):
    expected = (
        struct.pack("BBB", addr, 0x2B, success_mei_type)
        + data_pfx
        + (b"\xFF" * (8 - len(data_pfx)))
    )
    if response != expected:
        print(error + ", response: " + bh(response))
        raise BadMEIResponse()


def start_programming(addr):
    print("Send start programming...")
    response = mei_command(addr, 0x70, timeout=10000)
    mei_expect(response, addr, b"\xB0", "Start programming failed")
    print("Start programming succeeded.")


def get_challenge(addr):
    print("Send get seed")
    response = mei_command(addr, 0x27, timeout=3000)
    expected = struct.pack("BBBB", addr, 0x2B, 0x71, 0x67)
    if response[: len(expected)] != expected:
        print("Bad response to get seed: " + bh(response))
        raise BadMEIResponse()
    challenge = response[len(expected) : len(expected) + 4]
    print("Got seed: " + bh(challenge))
    return challenge


def send_key(addr, key):
    print("Send key")
    response = mei_command(addr, 0x28, data=key, timeout=3000)
    mei_expect(response, addr, b"\x68", "Start programming failed")
    print("Send key successful.")


def delta_seccalckey(challenge):
    (seed,) = struct.unpack(">L", challenge)
    for _ in range(32):
        if seed & 1 != 0:
            seed = seed ^ 0xC758A5B6
        seed = (seed >> 1) & 0x7FFFFFFF
    seed = seed ^ 0x06854137
    return struct.pack(">L", seed)


def verify_flash(addr):
    print("Verifying program...")
    response = mei_command(addr, 0x76, timeout=60000)
    mei_expect(response, addr, b"\xB6", "Program verification failed")


def set_write_address(psu_addr, flash_addr):
    # print("Set write address to " + hex(flash_addr))
    data = struct.pack(">LB", flash_addr, 0xEA)
    response = mei_command(psu_addr, 0x61, data=data, timeout=3000)
    mei_expect(response, psu_addr, b"\xA1\xEA", "Set address failed")


def write_data(addr, data):
    assert len(data) == 8
    command = struct.pack(">BBB", addr, 0x2B, 0x65) + data
    try:
        response = pyrmd.modbuscmd_sync(command, expected=13, timeout=3000)
    except pyrmd.ModbusCRCError:
        # This is not ideal, but upgrades in some Delta racks never complete
        # without it.
        # Suspicion is that we occasionally get replies from more than one unit
        # when we send a write (another unit's address byte is contained in our
        # request. It should look for silence before the start of a message,
        # but may not be perfect at that. It won't be in bootloader mode, so
        # will just send an error response, but that will be enough to collide
        # with and corrupt the real response we want to read. The content of
        # that response is only needed as an acknowledgement our request was
        # seen, so when this happens, rather than bail on the whole upgrade,
        # just assume that our write went through and send the next block, but
        # give some grace time for good measure.
        print("CRC check failure reading reply, continuing anyway...")
        time.sleep(1.0)
        return
    except pyrmd.ModbusTimeout:
        # Again, not ideal, we sometimes see timeouts. At this point
        # we hope that the PSU received the packet and we just did not
        # read the packet in time. So we can ignore the error.
        # We will catch a real issue during the verify phase.
        print("Timeout detected. Continuing anyway...")
        time.sleep(1.0)
        return
    expected = struct.pack(">B", addr) + b"\x2b\x73\xf0\xaa\xff\xff\xff\xff\xff\xff"
    if response != expected:
        print("Bad response to writing data: " + bh(response))
        raise BadMEIResponse()


def send_image(addr, fwimg):
    global statuspath
    total_chunks = sum([len(s) for s in fwimg.segments]) / 8
    sent_chunks = 0
    for s in fwimg.segments:
        if len(s) == 0:
            continue
        print("Sending " + str(s))
        set_write_address(addr, s.start_address)
        for i in range(0, len(s), 8):
            chunk = s.data[i : i + 8]
            if len(chunk) < 8:
                chunk = chunk + (b"\xFF" * (8 - len(chunk)))
            sent_chunks += 1
            # dont fill the restapi log with junk
            if statuspath is None:
                print(
                    "\r[%.2f%%] Sending chunk %d of %d..."
                    % (sent_chunks * 100.0 / total_chunks, sent_chunks, total_chunks),
                    end="",
                )
            sys.stdout.flush()
            write_data(addr, bytearray(chunk))
            status["flash_progress_percent"] = sent_chunks * 100.0 / total_chunks
            write_status()
        print("")


def reset_psu(addr):
    print("Resetting PSU...")
    try:
        response = mei_command(addr, 0x72, timeout=10000)
    except pyrmd.ModbusTimeout:
        print("No reply from PSU reset (expected.)")
        return
    expected = struct.pack(">BBBB", addr, 0x2B, 0x71, 0xB2) + (b"\xFF" * 7)
    if response != expected:
        print("Bad response to unit reset request: " + bh(response))
        raise BadMEIResponse()


def erase_flash(addr):
    print("Erasing flash... ")
    sys.stdout.flush()
    response = mei_command(addr, 0x65, timeout=30000)
    expected = struct.pack(">BBBB", addr, 0x2B, 0x71, 0xA5) + (b"\xFF" * 7)
    if response != expected:
        print("Bad response to erasing flash: " + bh(response))
        raise BadMEIResponse()


def update_psu(addr, filename):
    status_state("pausing_monitoring")
    pyrmd.pause_monitoring_sync()
    status_state("parsing_fw_file")
    fwimg = hexfile.load(filename)
    status_state("pre_handshake_reset")
    # This brings us back to the top of the bootloader state machine if we were
    # in bootloader, and should do nothing otherwise
    reset_psu(addr)
    time.sleep(5.0)
    status_state("bootloader_handshake")
    enter_bootloader(addr)
    start_programming(addr)
    challenge = get_challenge(addr)
    send_key(addr, delta_seccalckey(challenge))
    for _ in range(5):
        try:
            status_state("erase_flash")
            erase_flash(addr)
            status_state("flashing")
            send_image(addr, fwimg)
            status_state("verifying")
            verify_flash(addr)
            break
        except Exception:
            print("Problems detected in programming. Retrying from erase")
    status_state("resetting")
    reset_psu(addr)
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
            update_psu(args.addr, args.file)
        except Exception as e:
            print("Firmware update failed %s" % str(e))
            traceback.print_exc()
            global status
            status["exception"] = traceback.format_exc()
            status_state("failed")
            pyrmd.resume_monitoring_sync()
            if args.rmfwfile:
                os.remove(args.file)
            sys.exit(1)
        pyrmd.resume_monitoring_sync()
        if args.rmfwfile:
            os.remove(args.file)
        sys.exit(0)


if __name__ == "__main__":
    main()
