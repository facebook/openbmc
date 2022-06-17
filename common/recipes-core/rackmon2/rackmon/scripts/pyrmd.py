#!/usr/bin/env python3
"""
Modbus/rackmond library + Standalone tool to view current power/current
readings
"""
import socket
import asyncio
import contextlib
import json
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
    request = json.dumps(cmd).encode()
    if not os.path.exists("/var/run/rackmond.sock"):
        raise ModbusException()
    reader, writer = await asyncio.open_unix_connection("/var/run/rackmond.sock")
    req_header = struct.pack("@H", len(request))
    writer.write(req_header + request)
    response = await reader.read()
    (resp_len,) = struct.unpack("@H", response[:2])
    if len(response[2:]) != resp_len:
        print("WARNING: length mismatch: ", len(response[2:]), resp_len)
    return json.loads(response[2:].decode())


def rackmon_command_sync(cmd):
    request = json.dumps(cmd).encode()
    if not os.path.exists("/var/run/rackmond.sock"):
        raise ModbusException()
    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.connect("/var/run/rackmond.sock")
    req_header = struct.pack("@H", len(request))
    client.send(req_header + request)
    response = client.recv(65535)
    client.close()
    (resp_len,) = struct.unpack("@H", response[:2])
    if len(response[2:]) != resp_len:
        print("WARNING: length mismatch: ", len(response[2:]), resp_len)
    return json.loads(response[2:].decode())


async def modbuscmd(raw_cmd, expected=0, timeout=0):
    # Convert to integer array.
    raw_cmd_ints = list(raw_cmd)
    cmd = {
        "type": "raw",
        "cmd": raw_cmd_ints,
        "response_length": expected,
        "timeout": timeout,
    }
    result = await rackmon_command(cmd)
    if result["status"] != "SUCCESS":
        if result["status"] == "CRC_ERROR":
            log("<- crc check failure")
            raise ModbusCRCError()
        elif result["status"] == "TIMEOUT_ERROR":
            log("<- timeout")
            raise ModbusTimeout()
        else:
            raise ModbusException()
    log("<- {}".format(result))
    # Return everything but the CRC
    return bytes(bytearray(result["data"][:-2]))


async def pause_monitoring():
    cmd = {"type": "pause"}
    result = await rackmon_command(cmd)
    if result["status"] != "SUCCESS":
        print("Pause failed: %s" % (result["status"]))


async def resume_monitoring():
    cmd = {"type": "resume"}
    result = await rackmon_command(cmd)
    if result["status"] != "SUCCESS":
        print("Resume failed: %s" % (result["status"]))


async def read_register(addr, register, length=1, timeout=0):
    cmd = struct.pack(">BBHH", addr, 0x3, register, length)
    data = await modbuscmd(cmd, expected=5 + (2 * length), timeout=timeout)
    return data[3:]


def modbuscmd_sync(raw_cmd, expected=0, timeout=0):
    # Convert to integer array.
    raw_cmd_ints = list(raw_cmd)
    cmd = {
        "type": "raw",
        "cmd": raw_cmd_ints,
        "response_length": expected,
        "timeout": timeout,
    }
    result = rackmon_command_sync(cmd)
    if result["status"] != "SUCCESS":
        if result["status"] == "CRC_ERROR":
            log("<- crc check failure")
            raise ModbusCRCError()
        elif result["status"] == "TIMEOUT_ERROR":
            log("<- timeout")
            raise ModbusTimeout()
        else:
            raise ModbusException()
    log("<- {}".format(result))
    # Return everything but the CRC
    return bytes(bytearray(result["data"][:-2]))


def pause_monitoring_sync():
    cmd = {"type": "pause"}
    result = rackmon_command_sync(cmd)
    if result["status"] != "SUCCESS":
        print("Pause failed: %s" % (result["status"]))


def resume_monitoring_sync():
    cmd = {"type": "resume"}
    result = rackmon_command_sync(cmd)
    if result["status"] != "SUCCESS":
        print("Resume failed: %s" % (result["status"]))


def read_register_sync(addr, register, length=1, timeout=0):
    cmd = struct.pack(">BBHH", addr, 0x3, register, length)
    data = modbuscmd_sync(cmd, expected=5 + (2 * length), timeout=timeout)
    return data[3:]
