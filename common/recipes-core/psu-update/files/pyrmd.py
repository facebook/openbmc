#!/usr/bin/env python3
"""
Modbus/rackmond library + Standalone tool to view current power/current
readings
"""
import asyncio
import contextlib
import os
import struct
import time


class ModbusException(Exception):
    ...


class ModbusTimeout(ModbusException):
    ...


class ModbusCRCError(ModbusException):
    ...


class ModbusUnknownError(ModbusException):
    ...


modbuslog = None
logstart = None


def log(*args, **kwargs):
    if modbuslog:
        t = time.time() - logstart
        if all(args):
            pfx = "[{:4.02f}]".format(t)
        else:
            pfx = ""
        print(pfx, *args, file=modbuslog, **kwargs)


@contextlib.contextmanager
def transcript():
    global modbuslog, logstart
    logstart = time.time()
    with open("modbus-transcript.log", "w") as f:
        modbuslog = f
        yield
        modbuslog = None


async def rackmon_command(cmd):
    if not os.path.exists("/var/run/rackmond.sock"):
        raise ModbusException()
    reader, writer = await asyncio.open_unix_connection("/var/run/rackmond.sock")
    req_header = struct.pack("@H", len(cmd))
    writer.write(req_header + cmd)
    response = await reader.read()
    writer.close()
    return response


async def modbuscmd(raw_cmd, expected=0, timeout=0):
    cmd_header = struct.pack("@HxxHHL", 1, len(raw_cmd), expected, timeout)
    log("-> {}".format(" ".join("{:02x}".format(b) for b in raw_cmd)))
    cmd = cmd_header + raw_cmd
    response = await rackmon_command(cmd)
    (rlen,) = struct.unpack("@H", response[:2])
    if rlen == 0:
        (error,) = struct.unpack("@H", response[2:4])
        if error == 4:
            log("<- timeout")
            log("")
            raise ModbusTimeout()
        if error == 5:
            log("<- [CRC ERROR]")
            log("")
            raise ModbusCRCError()
        raise ModbusException(error)
    log("<- {}".format(" ".join("{:02x}".format(b) for b in response[2:rlen])))
    log("")
    return response[2:rlen]


async def pause_monitoring():
    COMMAND_TYPE_PAUSE_MONITORING = 0x04
    command = struct.pack("@Hxx", COMMAND_TYPE_PAUSE_MONITORING)
    result = await rackmon_command(command)
    (res_n,) = struct.unpack("@B", result)
    if res_n == 1:
        print("Monitoring was already paused when tried to pause")
    elif res_n == 0:
        print("Monitoring paused")
    else:
        print("Unknown response pausing monitoring: %d" % res_n)


async def resume_monitoring():
    COMMAND_TYPE_START_MONITORING = 0x05
    command = struct.pack("@Hxx", COMMAND_TYPE_START_MONITORING)
    result = await rackmon_command(command)
    (res_n,) = struct.unpack("@B", result)
    if res_n == 1:
        print("Monitoring was already running when tried to resume")
    elif res_n == 0:
        print("Monitoring resumed")
    else:
        print("Unknown response resuming monitoring: %d" % res_n)


async def read_register(addr, register, length=1, timeout=0):
    cmd = struct.pack(">BBHH", addr, 0x3, register, length)
    data = await modbuscmd(cmd, expected=5 + (2 * length), timeout=timeout)
    return data[3:]


def modbuscmd_sync(raw_cmd, expected=0, timeout=0):
    return asyncio.get_event_loop().run_until_complete(
        modbuscmd(raw_cmd, expected, timeout)
    )


def pause_monitoring_sync():
    return asyncio.get_event_loop().run_until_complete(pause_monitoring())


def resume_monitoring_sync():
    return asyncio.get_event_loop().run_until_complete(resume_monitoring())


def read_register_sync(addr, register, length=1, timeout=0):
    return asyncio.get_event_loop().run_until_complete(
        read_register(addr, register, length, timeout)
    )
