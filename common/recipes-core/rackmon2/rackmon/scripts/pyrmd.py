#!/usr/bin/env python3
"""
Modbus/rackmond library + Standalone tool to view current power/current
readings
"""
import asyncio
import contextlib
import json
import os
import socket
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


class RackmonInterface:
    @classmethod
    def _check(cls, resp):
        status = resp["status"]
        if status == "SUCCESS":
            return
        if status == "CRC_ERROR":
            log("<- crc check failure")
            raise ModbusCRCError()
        elif status == "TIMEOUT_ERROR":
            log("<- timeout")
            raise ModbusTimeout()
        else:
            log("<-", status)
            raise ModbusException(status)

    @classmethod
    def _write(cls, addr, register, data, timeout):
        cmd = {
            "devAddress": addr,
            "regAddress": register,
            "timeout": timeout,
        }
        if isinstance(data, int):
            cmd["type"] = "writeSingleRegister"
        elif isinstance(data, list):
            cmd["type"] = "presetMultipleRegisters"
        else:
            raise ValueError("What the")
        cmd["regValue"] = data
        return cmd

    @classmethod
    def _read(cls, addr, register, length, timeout):
        return {
            "type": "readHoldingRegisters",
            "devAddress": addr,
            "regAddress": register,
            "numRegisters": length,
            "timeout": timeout,
        }

    @classmethod
    def _raw(cls, raw_cmd, expected, timeout):
        # Convert to integer array.
        raw_cmd_ints = list(raw_cmd)
        cmd = {
            "type": "raw",
            "cmd": raw_cmd_ints,
            "response_length": expected,
            "timeout": timeout,
        }
        return cmd

    @classmethod
    def _rawResp(cls, response, fullResp, isBytes):
        if fullResp:
            ret = response["data"]
        else:
            ret = response["data"][:-2]
        if isBytes:
            return bytes(ret)
        return ret

    @classmethod
    def _pause(cls):
        return {"type": "pause"}

    @classmethod
    def _resume(cls):
        return {"type": "resume"}

    @classmethod
    def _list(cls):
        return {"type": "listModbusDevices"}

    @classmethod
    def _data(cls, raw, dataFilter=None):
        if raw:
            return {"type": "getMonitorDataRaw"}
        req = {"type": "getMonitorData"}
        if dataFilter is not None:
            req = {**req, **dataFilter}
        return req

    @classmethod
    def _execute(cls, cmd):
        request = json.dumps(cmd).encode()
        if not os.path.exists("/var/run/rackmond.sock"):
            raise ModbusException()
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        client.connect("/var/run/rackmond.sock")
        req_header = struct.pack("@H", len(request))
        client.send(req_header + request)
        response = bytes()

        def recvExact(num):
            received = 0
            chunk = bytes()
            while received < num:
                chunk += client.recv(num - received)
                received = len(chunk)
            return chunk

        def recvChunk():
            chunk_len_b = recvExact(2)
            (chunk_len,) = struct.unpack("@H", chunk_len_b)
            chunk = recvExact(chunk_len)
            return chunk, len(chunk) == 0xFFFF

        while True:
            data, cont_recv = recvChunk()
            response += data
            if not cont_recv:
                break
        client.close()
        return json.loads(response.decode())

    @classmethod
    def _do(cls, f, *args, **kwargs):
        cmd = f(*args, **kwargs)
        resp = cls._execute(cmd)
        cls._check(resp)
        return resp

    @classmethod
    def raw(cls, raw_cmd, expected, timeout=0, fullResp=False):
        result = cls._do(cls._raw, raw_cmd, expected, timeout)
        log("<- {}".format(result))
        return cls._rawResp(result, fullResp, isinstance(raw_cmd, bytes))

    @classmethod
    def pause(cls):
        cls._do(cls._pause)

    @classmethod
    def resume(cls):
        cls._do(cls._resume)

    @classmethod
    def list(cls):
        result = cls._do(cls._list)
        return result["data"]

    @classmethod
    def data(cls, raw=True, dataFilter=None):
        result = cls._do(cls._data, raw, dataFilter)
        return result["data"]

    @classmethod
    def read(cls, addr, reg, length=1, timeout=0):
        result = cls._do(cls._read, addr, reg, length, timeout)
        return result["regValues"]

    @classmethod
    def write(cls, addr, reg, data, timeout=0):
        cls._do(cls._write, addr, reg, data, timeout)


class RackmonAsyncInterface(RackmonInterface):
    @classmethod
    async def _execute(cls, cmd):
        request = json.dumps(cmd).encode()
        if not os.path.exists("/var/run/rackmond.sock"):
            raise ModbusException()
        reader, writer = await asyncio.open_unix_connection("/var/run/rackmond.sock")
        req_header = struct.pack("@H", len(request))
        writer.write(req_header + request)

        async def recvExact(num):
            received = 0
            chunk = bytes()
            while received < num:
                chunk += await reader.read(num - received)
                received = len(chunk)
            return chunk

        async def recvChunk():
            chunk_len_b = await recvExact(2)
            (chunk_len,) = struct.unpack("@H", chunk_len_b)
            chunk = await recvExact(chunk_len)
            return chunk, len(chunk) == 0xFFFF

        response = bytes()
        while True:
            data, cont_recv = await recvChunk()
            response += data
            if not cont_recv:
                break

        writer.close()
        return json.loads(response.decode())

    @classmethod
    async def _do(cls, f, *args, **kwargs):
        cmd = f(*args, **kwargs)
        resp = await cls._execute(cmd)
        cls._check(resp)
        return resp

    @classmethod
    async def raw(cls, raw_cmd, expected, timeout=0, fullResp=False):
        result = await cls._do(cls._raw, raw_cmd, expected, timeout)
        log("<- {}".format(result))
        return cls._rawResp(result, fullResp, isinstance(raw_cmd, bytes))

    @classmethod
    async def pause(cls):
        await cls._do(cls._pause)

    @classmethod
    async def resume(cls):
        await cls._do(cls._resume)

    @classmethod
    async def list(cls):
        result = await cls._do(cls._list)
        return result["data"]

    @classmethod
    async def data(cls, raw=True, dataFilter=None):
        result = await cls._do(cls._data, raw, dataFilter)
        return result["data"]

    @classmethod
    async def read(cls, addr, reg, length=1, timeout=0):
        result = await cls._do(cls._read, addr, reg, length, timeout)
        return result["regValues"]

    @classmethod
    async def write(cls, addr, reg, data, timeout=0):
        await cls._do(cls._write, addr, reg, data, timeout)
