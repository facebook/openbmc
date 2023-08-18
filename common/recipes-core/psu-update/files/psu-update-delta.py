#!/usr/bin/env python3

import struct
import sys
import time
import traceback

import hexfile
from modbus_update_helper import (
    auto_int,
    bh,
    get_parser,
    print_perc,
    retry,
    suppress_monitoring,
)
from pyrmd import (
    ModbusCRCError,
    ModbusException,
    ModbusTimeout,
    RackmonInterface as rmd,
)


parser = get_parser()
parser.add_argument("--key", type=auto_int, required=True, help="Sec key")


class BadMEIResponse(ModbusException):
    ...


def mei_command(addr, func_code, mei_type=0x64, data=None, timeout=0):
    i_data = data
    if i_data is None:
        i_data = b"\xFF" * 7
    if len(i_data) < 7:
        i_data = i_data + (b"\xFF" * (7 - len(i_data)))
    assert len(i_data) == 7
    command = struct.pack("BBBB", addr, 0x2B, mei_type, func_code) + i_data
    return rmd.raw(command, expected=13, timeout=timeout)


def enter_bootloader(addr):
    try:
        print("Entering bootloader...")
        mei_command(addr, 0xFB, timeout=4000)
    except ModbusTimeout:
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


def delta_seccalckey(challenge, key):
    lower = key & 0xFFFFFFFF
    upper = (key >> 32) & 0xFFFFFFFF
    (seed,) = struct.unpack(">L", challenge)
    for _ in range(32):
        if seed & 1 != 0:
            seed = seed ^ lower
        seed = (seed >> 1) & 0x7FFFFFFF
    seed = seed ^ upper
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
        response = rmd.raw(command, expected=13, timeout=3000)
    except ModbusCRCError:
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
    except ModbusTimeout:
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
            print_perc(
                sent_chunks * 100.0 / total_chunks,
                "Sending chunk %d of %d..." % (sent_chunks, total_chunks),
            )
            write_data(addr, bytearray(chunk))
        print_perc(100.0, "Sending chunk %d of %d..." % (total_chunks, total_chunks))


def reset_psu(addr):
    print("Resetting PSU...")
    try:
        response = mei_command(addr, 0x72, timeout=10000)
    except ModbusTimeout:
        print("No reply from PSU reset (expected.)")
        return
    expected = struct.pack(">BBBB", addr, 0x2B, 0x71, 0xB2) + (b"\xFF" * 7)
    if response != expected:
        print("Bad response to unit reset request: " + bh(response))
        raise BadMEIResponse()


def erase_flash(addr):
    print("Erasing flash... ")
    response = mei_command(addr, 0x65, timeout=30000)
    expected = struct.pack(">BBBB", addr, 0x2B, 0x71, 0xA5) + (b"\xFF" * 7)
    if response != expected:
        print("Bad response to erasing flash: " + bh(response))
        raise BadMEIResponse()


@retry(5)
def program_flash(addr, fwimg):
    erase_flash(addr)
    send_image(addr, fwimg)
    verify_flash(addr)


def update_psu(addr, filename, key):
    print("Parsing Firmware...")
    fwimg = hexfile.load(filename)
    # This brings us back to the top of the bootloader state machine if we were
    # in bootloader, and should do nothing otherwise
    reset_psu(addr)
    time.sleep(5.0)
    enter_bootloader(addr)
    start_programming(addr)
    challenge = get_challenge(addr)
    send_key(addr, delta_seccalckey(challenge, key))
    program_flash(addr, fwimg)
    reset_psu(addr)
    time.sleep(30.0)


def main():
    args = parser.parse_args()
    with suppress_monitoring():
        try:
            update_psu(args.addr, args.file, args.key)
        except Exception as e:
            print("Firmware update failed %s" % str(e))
            traceback.print_exc()
            sys.exit(1)


if __name__ == "__main__":
    main()
