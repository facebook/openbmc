from contextlib import contextmanager

import minimalmodbus
from modbus_common import (
    get_pmm_addr,
    ModbusCRCError,
    ModbusException,
    ModbusInvalidArgs,
    ModbusTimeout,
    ModbusUnknownError,
)
from modbus_monitor import MonitorChain, NullMonitor, PhosphorModbusMonitor, PmmMonitor

# Re-exported so users of this module keep catching the exceptions off
# the backend they imported. Both backends raise the same classes.
__all__ = [
    "ModbusCRCError",
    "ModbusException",
    "ModbusInvalidArgs",
    "ModbusTimeout",
    "ModbusUnknownError",
    "Modbus",
]

# Longest possible modbus RTU frame. Used when the caller does not know how
# large the response is going to be: read up to a full frame and let the
# serial timeout end the transaction.
MODBUS_RTU_MAX_FRAME = 256

# Used when the caller does not ask for one. pyrmd spells "use the
# default" as timeout=0, which pyserial would instead read as "never
# block", so 0 maps here too.
DEFAULT_TIMEOUT_MS = 300

# A PMM runs at fixed line settings, whatever the devices behind it use.
PMM_BAUDRATE = 115200
PMM_PARITY = "E"

# Parity keyed both by the single character phosphor_modbus spells it
# with (see DeviceConfig.parity_char) and by the long form.
PARITY = {
    "N": minimalmodbus.serial.PARITY_NONE,
    "NONE": minimalmodbus.serial.PARITY_NONE,
    "E": minimalmodbus.serial.PARITY_EVEN,
    "EVEN": minimalmodbus.serial.PARITY_EVEN,
    "O": minimalmodbus.serial.PARITY_ODD,
    "ODD": minimalmodbus.serial.PARITY_ODD,
}


def decode_parity(parity):
    """pyserial parity constant from either spelling, case insensitive"""
    try:
        return PARITY[str(parity).upper()]
    except KeyError:
        raise ValueError(
            "Unknown parity %r, expected one of %s"
            % (parity, ", ".join(sorted(PARITY)))
        ) from None


def stopbits_for(parity):
    """
    A modbus RTU character is always 11 bits. When there is no parity
    bit a second stop bit takes its place (MODBUS over Serial Line spec
    v1.02, 2.5.1).
    """
    if parity == minimalmodbus.serial.PARITY_NONE:
        return minimalmodbus.serial.STOPBITS_TWO
    return minimalmodbus.serial.STOPBITS_ONE


@contextmanager
def _translate_errors():
    """
    Map minimalmodbus's exceptions onto the backend independent ones.

    Anything not listed keeps its own type on purpose: callers rely on
    ValueError and friends reaching them intact, and turning every
    failure into a bare Exception would hide which one it was.
    """
    try:
        yield
    except minimalmodbus.SlaveReportedException as e:
        # The message names the modbus error code (illegal function,
        # illegal data address, NAK, busy, ...), so keep it.
        raise ModbusException(str(e)) from e
    except minimalmodbus.NoResponseError as e:
        raise ModbusTimeout() from e
    except minimalmodbus.InvalidResponseError as e:
        # Covers a bad CRC as well as the other ways a frame can be
        # malformed (too short, wrong slave address). Callers only ever
        # special case the CRC, and minimalmodbus does not separate them
        # into distinct types.
        raise ModbusCRCError() from e
    except minimalmodbus.LocalEchoError as e:
        raise ModbusUnknownError() from e


class Modbus:
    """
    Handle to a single modbus device reached over a serial port directly.

    dev_addr is the device address; only its low byte is significant. The
    'unique' addresses the rackmon backend uses to disambiguate devices
    sharing a wire address have no meaning here, as a handle is already
    bound to one port.

    pmm is the PMM which owns this device, or None if the device is not
    behind one. It shares this device's port.

    monitor is who polls this device and has to be told to stand off
    while we drive it, phosphor-modbus unless told otherwise -- pass a
    RackmonMonitor for a device this backend drives but rackmond
    monitors. Whatever it is, the PMM's own monitoring is suppressed
    along with it.
    """

    def __init__(self, dev_addr, baudrate, parity, devpath, monitor=None):
        self.addr = self.dev_addr = dev_addr & 0xFF
        self.addr_b = self.addr.to_bytes(1, "big")
        self.devpath = devpath
        self.baudrate = baudrate
        self.parity = decode_parity(parity)
        self.stopbits = stopbits_for(self.parity)

        self.pmm_addr = get_pmm_addr(self.dev_addr)
        self.pmm = None
        if self.pmm_addr is not None:
            # The PMM is suppressed as part of this device's monitor,
            # and shares its port and so its exclusion, so its own
            # handle needs no monitor of its own.
            self.pmm = Modbus(
                self.pmm_addr,
                PMM_BAUDRATE,
                PMM_PARITY,
                self.devpath,
                monitor=NullMonitor(),
            )

        if monitor is None:
            monitor = PhosphorModbusMonitor(self.devpath)
        self.monitor = MonitorChain(monitor, PmmMonitor(self.pmm) if self.pmm else None)

        self.dev = minimalmodbus.Instrument(
            self.devpath, self.dev_addr, mode=minimalmodbus.MODE_RTU
        )

    def __str__(self):
        desc = (
            f"MinimalModbus({self.dev_addr}:B{self.baudrate}"
            f":P{self.parity} @ {self.devpath}"
        )
        if self.pmm is not None:
            desc += f" PMM={self.pmm}"
        return desc + ")"

    def _select(self, timeout):
        """
        minimalmodbus caches one serial.Serial per device path, so every
        Instrument on the same bus (this device and its PMM) shares the same
        port settings. Apply ours right before each transaction rather than
        once at construction, else whichever device was constructed last
        wins for everyone.
        """
        self.dev.serial.baudrate = self.baudrate
        self.dev.serial.bytesize = minimalmodbus.serial.EIGHTBITS
        self.dev.serial.parity = self.parity
        self.dev.serial.stopbits = self.stopbits
        self.dev.serial.timeout = (timeout or DEFAULT_TIMEOUT_MS) / 1000.0

    def read(self, reg, length=1, timeout=DEFAULT_TIMEOUT_MS):
        self._select(timeout)
        with _translate_errors():
            return self.dev.read_registers(reg, length)

    def read_str(self, reg, length=1, timeout=DEFAULT_TIMEOUT_MS):
        self._select(timeout)
        with _translate_errors():
            return self.dev.read_string(reg, length).rstrip("\x00\x20")

    def write(self, reg, data, timeout=DEFAULT_TIMEOUT_MS):
        # Callers pass either one register value or a sequence of them,
        # minimalmodbus only takes a list.
        values = [data] if isinstance(data, int) else list(data)
        self._select(timeout)
        with _translate_errors():
            self.dev.write_registers(reg, values)

    def raw(self, req, expected=0, timeout=DEFAULT_TIMEOUT_MS):
        self._select(timeout)
        # minimalmodbus has no public raw API and represents frames as
        # latin-1 encoded str internally (except for _communicate() which
        # wants bytes), so encode/decode around its helpers.
        RAW_ENCODING = "latin1"
        nbytes = expected if expected > 0 else MODBUS_RTU_MAX_FRAME
        with _translate_errors():
            wreq = minimalmodbus._embed_payload(
                self.addr, minimalmodbus.MODE_RTU, req[0], req[1:].decode(RAW_ENCODING)
            )
            wrsp = self.dev._communicate(wreq.encode(RAW_ENCODING), nbytes)
            resp = minimalmodbus._extract_payload(
                wrsp.decode(RAW_ENCODING), self.addr, minimalmodbus.MODE_RTU, req[0]
            )
            # Re-add the function code so the callers can use existing checks.
            return req[:1] + resp.encode(RAW_ENCODING)

    def suppress_monitoring(self):
        """
        contextmanager to pause monitoring of this device on entry and
        resume on exit, including exits due to exception
        """
        return self.monitor.suppress()
