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
    def __init__(self):
        super().__init__("ERR_TIMEOUT")


class ModbusCRCError(ModbusException):
    def __init__(self):
        super().__init__("ERR_BAD_CRC")


class ModbusUnknownError(ModbusException):
    def __init__(self):
        super().__init__("ERR_IO_FAILURE")


class ModbusInvalidArgs(ModbusException):
    def __init__(self):
        super().__init__("ERR_INVALID_ARGS")


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
    def _setTimeout(cls, req, timeout):
        if timeout > 0:
            req["timeout"] = timeout

    @classmethod
    def _check(cls, resp):
        status = resp["status"]
        if status == "SUCCESS":
            return
        if status == "ERR_BAD_CRC":
            log("<- crc check failure")
            raise ModbusCRCError()
        elif status == "ERR_TIMEOUT":
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
        }
        cls._setTimeout(cmd, timeout)
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
        cmd = {
            "type": "readHoldingRegisters",
            "devAddress": addr,
            "regAddress": register,
            "numRegisters": length,
        }
        cls._setTimeout(cmd, timeout)
        return cmd

    @classmethod
    def _read_file(cls, addr, records, timeout):
        cmd = {"type": "readFileRecord", "devAddress": addr, "records": records}
        cls._setTimeout(cmd, timeout)
        return cmd

    @classmethod
    def _raw(cls, raw_cmd, expected, timeout):
        # Convert to integer array.
        raw_cmd_ints = list(raw_cmd)
        cmd = {"type": "raw", "cmd": raw_cmd_ints, "response_length": expected}
        cls._setTimeout(cmd, timeout)
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
    def _rescan(cls):
        return {"type": "rescan"}

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
    def _execute(cls, cmd, decodeJson=True):
        request = json.dumps(cmd).encode()
        if not os.path.exists("/var/run/rackmond.sock"):
            raise ModbusException()
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        client.settimeout(30)
        client.connect("/var/run/rackmond.sock")
        req_header = struct.pack("@L", len(request))
        client.sendall(req_header + request)
        response = bytes()

        def recvExact(num):
            data = bytes()
            retries = 0
            maxRetries = 3
            while len(data) < num:
                chunk = client.recv(num - len(data))
                if chunk == b"":
                    retries += 1
                    if retries > maxRetries:
                        raise RuntimeError("Potentially connection closed")
                    continue
                retries = 0
                data += chunk
            return data

        (data_len,) = struct.unpack("@L", recvExact(4))
        response = recvExact(data_len)
        client.close()
        if decodeJson:
            return json.loads(response.decode())
        return response.decode()

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
    def rescan(cls):
        cls._do(cls._rescan)

    @classmethod
    def list(cls):
        result = cls._do(cls._list)
        return result["data"]

    @classmethod
    def data(cls, raw=True, dataFilter=None, decodeJson=True):
        if decodeJson:
            result = cls._do(cls._data, raw, dataFilter)
            return result["data"]
        cmd = cls._data(raw, dataFilter)
        return cls._execute(cmd, False)

    @classmethod
    def read(cls, addr, reg, length=1, timeout=0):
        result = cls._do(cls._read, addr, reg, length, timeout)
        return result["regValues"]

    @classmethod
    def write(cls, addr, reg, data, timeout=0):
        cls._do(cls._write, addr, reg, data, timeout)

    @classmethod
    def read_file(cls, addr, records, timeout=0):
        result = cls._do(cls._read_file, addr, records, timeout)
        return result["data"]


class RackmonAsyncInterface(RackmonInterface):
    @classmethod
    async def _execute(cls, cmd, decodeJson=True):
        return await asyncio.get_event_loop().run_in_executor(
            None, RackmonInterface._execute, cmd, decodeJson
        )

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
    async def rescan(cls):
        await cls._do(cls._rescan)

    @classmethod
    async def list(cls):
        result = await cls._do(cls._list)
        return result["data"]

    @classmethod
    async def data(cls, raw=True, dataFilter=None, decodeJson=True):
        if decodeJson:
            result = await cls._do(cls._data, raw, dataFilter)
            return result["data"]
        cmd = cls._data(raw, dataFilter)
        return await cls._execute(cmd, False)

    @classmethod
    async def read(cls, addr, reg, length=1, timeout=0):
        result = await cls._do(cls._read, addr, reg, length, timeout)
        return result["regValues"]

    @classmethod
    async def write(cls, addr, reg, data, timeout=0):
        await cls._do(cls._write, addr, reg, data, timeout)

    @classmethod
    async def read_file(cls, addr, records, timeout=0):
        result = await cls._do(cls._read_file, addr, records, timeout)
        return result["data"]
