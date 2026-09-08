"""
Fakes for the transports the modules under test import.

pyrmd (rackmon) and minimalmodbus are not importable on a build host, so
modbus_impl_pyrmd.py and modbus_impl_minimalmodbus.py cannot even be
imported without them. Importing this module first puts a fake of each
into sys.modules; a test then patches the individual calls it cares
about.

The fakes are installed unconditionally, so a host which does have the
real thing installed still runs the same tests. They implement only what
the modules under test use, and every transport call raises unless the
test stubbed it: a test which reaches the wire is a test which is not
saying what it means.
"""

import sys
import types


def _fake_crc(frame):
    """Stand-in for the CRC16 the real minimalmodbus appends"""
    total = sum(ord(c) for c in frame) & 0xFFFF
    return chr(total & 0xFF) + chr((total >> 8) & 0xFF)


def _install_pyrmd():
    mod = types.ModuleType("pyrmd")

    class ModbusException(Exception):
        pass

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

    def _unstubbed(name):
        def call(*args, **kwargs):
            raise AssertionError("pyrmd.RackmonInterface.%s() is not stubbed" % name)

        return classmethod(call)

    class RackmonInterface:
        read = _unstubbed("read")
        get = _unstubbed("get")
        write = _unstubbed("write")
        raw = _unstubbed("raw")
        list = _unstubbed("list")
        get_interface = _unstubbed("get_interface")
        pause = _unstubbed("pause")
        resume = _unstubbed("resume")

    mod.ModbusException = ModbusException
    mod.ModbusTimeout = ModbusTimeout
    mod.ModbusCRCError = ModbusCRCError
    mod.ModbusUnknownError = ModbusUnknownError
    mod.ModbusInvalidArgs = ModbusInvalidArgs
    mod.RackmonInterface = RackmonInterface
    sys.modules["pyrmd"] = mod
    return mod


def _fake_serial_module():
    serial = types.ModuleType("serial")
    serial.PARITY_NONE = "N"
    serial.PARITY_EVEN = "E"
    serial.PARITY_ODD = "O"
    serial.STOPBITS_ONE = 1
    serial.STOPBITS_TWO = 2
    serial.EIGHTBITS = 8
    return serial


def _add_minimalmodbus_exceptions(mod):
    """The exception hierarchy the backend catches"""

    class ModbusException(IOError):
        pass

    class SlaveReportedException(ModbusException):
        pass

    class InvalidResponseError(ModbusException):
        pass

    class NoResponseError(ModbusException):
        pass

    class LocalEchoError(ModbusException):
        pass

    mod.ModbusException = ModbusException
    mod.SlaveReportedException = SlaveReportedException
    mod.InvalidResponseError = InvalidResponseError
    mod.NoResponseError = NoResponseError
    mod.LocalEchoError = LocalEchoError


class FakeSerial:
    """The subset of serial.Serial the backend touches"""

    def __init__(self, port):
        self.port = port
        self.baudrate = None
        self.bytesize = None
        self.parity = None
        self.stopbits = None
        self.timeout = None


def _fake_instrument():
    class Instrument:
        # The real minimalmodbus caches one serial.Serial per port, so
        # every Instrument on the same bus shares its settings. The
        # backend's _select() exists because of that, so the fake has to
        # do it too.
        ports = {}
        instances = []

        def __init__(self, port, slaveaddress, mode=None):
            self.port = port
            self.address = slaveaddress
            self.mode = mode
            self.serial = Instrument.ports.setdefault(port, FakeSerial(port))
            Instrument.instances.append(self)

        @staticmethod
        def _unstubbed(name):
            def call(*args, **kwargs):
                raise AssertionError("Instrument.%s() is not stubbed" % name)

            return call

        def read_registers(self, *args, **kwargs):
            return Instrument._unstubbed("read_registers")()

        def read_string(self, *args, **kwargs):
            return Instrument._unstubbed("read_string")()

        def write_registers(self, *args, **kwargs):
            return Instrument._unstubbed("write_registers")()

        def _communicate(self, *args, **kwargs):
            return Instrument._unstubbed("_communicate")()

        @classmethod
        def reset(cls):
            cls.ports = {}
            cls.instances = []

    return Instrument


def _add_minimalmodbus_framing(mod):
    """The frame the backend builds by hand for a raw transaction"""

    def _embed_payload(slaveaddress, mode, functioncode, payloaddata):
        frame = chr(slaveaddress) + chr(functioncode) + payloaddata
        return frame + _fake_crc(frame)

    def _extract_payload(response, slaveaddress, mode, functioncode):
        if len(response) < 4:
            raise mod.InvalidResponseError("Too short response")
        body, crc = response[:-2], response[-2:]
        if crc != _fake_crc(body):
            raise mod.InvalidResponseError("Checksum error")
        if ord(body[0]) != slaveaddress:
            raise mod.InvalidResponseError("Wrong slave address")
        if ord(body[1]) != functioncode:
            raise mod.SlaveReportedException("Wrong function code")
        return body[2:]

    mod._embed_payload = _embed_payload
    mod._extract_payload = _extract_payload


def _install_minimalmodbus():
    mod = types.ModuleType("minimalmodbus")
    mod.serial = _fake_serial_module()
    mod.MODE_RTU = "rtu"
    _add_minimalmodbus_exceptions(mod)
    mod.Instrument = _fake_instrument()
    _add_minimalmodbus_framing(mod)
    sys.modules["minimalmodbus"] = mod
    return mod


pyrmd = _install_pyrmd()
minimalmodbus = _install_minimalmodbus()


class FakeDevice:
    """
    A modbus handle, as the updaters use one.

    read()/read_str()/write()/raw() return whatever the test queued for
    them and record every call, so a test can assert on the bytes an
    updater put on the wire without standing up a backend.
    """

    def __init__(self, dev_addr=0x1, raw=(), read=(), read_str=()):
        self.dev_addr = dev_addr
        self.addr = dev_addr & 0xFF
        self.calls = []
        self.raw_responses = list(raw)
        self.read_responses = list(read)
        self.read_str_responses = list(read_str)

    def _next(self, queue, name, args):
        if not queue:
            raise AssertionError("Unexpected %s%r, nothing queued" % (name, args))
        response = queue.pop(0)
        if isinstance(response, Exception):
            raise response
        if callable(response):
            return response(*args)
        return response

    def raw(self, req, expected=0, timeout=0):
        self.calls.append(("raw", req, expected, timeout))
        return self._next(self.raw_responses, "raw", (req,))

    def read(self, reg, length=1, timeout=0):
        self.calls.append(("read", reg, length, timeout))
        return self._next(self.read_responses, "read", (reg, length))

    def read_str(self, reg, length=1, timeout=0):
        self.calls.append(("read_str", reg, length, timeout))
        return self._next(self.read_str_responses, "read_str", (reg, length))

    def write(self, reg, data, timeout=0):
        self.calls.append(("write", reg, data, timeout))

    def calls_of(self, kind):
        return [call for call in self.calls if call[0] == kind]
