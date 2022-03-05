#!/usr/bin/env python3
"""
Modbus/rackmond library + Standalone tool to view current power/current
readings
"""
import asyncio
import contextlib
import struct
import time


class ModbusException(Exception):
    ...


class ModbusTimeout(ModbusException):
    ...


class ModbusCRCError(ModbusException):
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


# This many modbus requests can be in flight at once
MAX_QUEUED = 15
MODBUS_SEM = asyncio.Semaphore(MAX_QUEUED)


async def modbuscmd(raw_cmd, expected=0, timeout=0):
    async with MODBUS_SEM:
        reader, writer = await asyncio.open_unix_connection("/var/run/rackmond.sock")
        cmd_header = struct.pack("@HxxHHL", 1, len(raw_cmd), expected, timeout)
        log("-> {}".format(" ".join("{:02x}".format(b) for b in raw_cmd)))
        cmd = cmd_header + raw_cmd
        req_header = struct.pack("@H", len(cmd))
        writer.write(req_header + cmd)
        response = await reader.read()
        writer.close()
        rlen, = struct.unpack("@H", response[:2])
        if rlen == 0:
            error, = struct.unpack("@H", response[2:4])
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


async def read_register(addr, register, length=1, timeout=0):
    cmd = struct.pack(">BBHH", addr, 0x3, register, length)
    data = await modbuscmd(cmd, expected=5 + (2 * length), timeout=timeout)
    return data[3:]


class Register:
    __slots__ = ["name", "start", "length", "convert", "interval"]

    def __init__(self, name, start, length=1, convert=None, interval=None):
        self.name = name
        self.start = start
        self.length = length
        self.convert = convert
        self.interval = interval


def conv_ascii(bs):
    return bs.decode("ascii").strip("\x00 ")


def conv_fixed(n):
    def convert(bs):
        return int.from_bytes(bs, byteorder="big") / (2 ** n)

    return convert


REGISTERS = [
    Register("model", 0x0, 8, conv_ascii, 120),
    Register("fw", 0x38, 4, conv_ascii, 120),
    Register("power", 0x96, convert=conv_fixed(3)),
    Register("current", 0x8C, convert=conv_fixed(6)),
]


class PSU:
    def __init__(self, addr, rescan=120):
        self.addr = addr
        self.readings = {}
        self.regs = REGISTERS
        self.uptimes = {}
        self.rescan = rescan
        self.recheck = None

    async def update_register(self, reg):
        if reg.interval and time.time() < reg.interval + self.uptimes.get(reg.name, 0):
            return
        try:
            bs = await read_register(self.addr, reg.start, reg.length)
            if reg.convert:
                self.readings[reg.name] = reg.convert(bs)
            else:
                self.readings[reg.name] = bs
            self.uptimes[reg.name] = time.time()
            return True
        except ModbusTimeout:
            self.readings[reg.name] = None
            return False

    async def read(self):
        now = time.time()
        if self.recheck and now < self.recheck:
            return
        self.recheck = None
        if all(
            not ok
            for ok in await asyncio.gather(
                *[self.update_register(reg) for reg in self.regs]
            )
        ):
            # If all reads failed, wait `rescan` seconds to check this PSU
            # address again
            now = time.time()
            self.recheck = now + self.rescan


async def modelscan():
    addrs = [0xA4, 0xA5, 0xA6, 0xB4, 0xB5, 0xB6]
    psus = [PSU(a) for a in addrs]
    cols = [reg.name for reg in REGISTERS]
    widths = {}
    print("\x1b[?1049h", end="")  # enter altscreen
    print("\x1b[0;0H\x1b[s\x1b[?25l", end="")  # save cursor
    while True:
        # Update PSU data
        await asyncio.gather(*[psu.read() for psu in psus])
        print("    ", end="")
        for col in cols:
            print("{c:<{w}} ".format(c=col, w=widths.get(col, 0)), end="")
        print()
        for psu in psus:
            print(" {:02x} ".format(psu.addr), end="")
            for col in cols:
                v = psu.readings.get(col, None)
                if not v and col == cols[0] and psu.recheck:
                    v = "t/o.. ({:0.02f}s)".format(psu.recheck - time.time())
                v = str(v) if v else "t/o"
                widths[col] = max(widths.get(col, 0), len(v))
                print("{v:<{w}} ".format(v=v, w=widths[col]), end="")
            print()
        print("\x1b[u", end="", flush=True)  # restore cursor


if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    try:
        loop.run_until_complete(modelscan())
    except KeyboardInterrupt:
        pass
    finally:
        print("\x1b[?1049l\x1b[?25h", end="", flush=True)  # leave altscreen
        loop.run_until_complete(loop.shutdown_asyncgens())
        loop.close()
