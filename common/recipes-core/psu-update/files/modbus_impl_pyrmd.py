from contextlib import contextmanager

import pyrmd
from modbus_common import (
    decode_modbus_address,
    get_pmm_addr,
    ModbusCRCError,
    ModbusException,
    ModbusInvalidArgs,
    ModbusTimeout,
    ModbusUnknownError,
    PMM_PAUSE_REG,
)
from modbus_monitor import MonitorChain, NullMonitor, PmmMonitor, RackmonMonitor

# Re-exported so users of this module keep catching the exceptions off
# the backend they imported. Both backends raise the same classes.
__all__ = [
    "ModbusCRCError",
    "ModbusException",
    "ModbusInvalidArgs",
    "ModbusTimeout",
    "ModbusUnknownError",
    "Modbus",
    "PMM_PAUSE_REG",
    "decode_modbus_address",
    "get_pmm_addr",
]


@contextmanager
def _translate_errors():
    """
    Map pyrmd's exceptions onto the backend independent ones. The
    subclasses have to be caught before pyrmd.ModbusException, which is
    their common base.
    """
    try:
        yield
    except pyrmd.ModbusTimeout as e:
        raise ModbusTimeout() from e
    except pyrmd.ModbusCRCError as e:
        raise ModbusCRCError() from e
    except pyrmd.ModbusUnknownError as e:
        raise ModbusUnknownError() from e
    except pyrmd.ModbusInvalidArgs as e:
        raise ModbusInvalidArgs() from e
    except pyrmd.ModbusException as e:
        # The message is the rackmond status string, keep it.
        raise ModbusException(str(e)) from e


class Modbus:
    """
    Handle to a single modbus device.

    dev_addr is the device address as provided by the user. Addresses larger
    than a byte are 'unique' addresses: the low byte is what goes on the
    wire (addr/addr_b) and the full address is used to disambiguate
    devices sharing the same wire address.

    pmm is the address of the PMM which owns this device, or None if the
    device is not behind one.

    monitor is who polls this device and has to be told to stand off
    while we drive it, rackmond unless told otherwise. Whatever it is,
    the PMM's own monitoring is suppressed along with it.
    """

    def __init__(self, dev_addr, monitor=None):
        self.dev_addr = dev_addr
        self.addr, self.addr_b, self.unique_addr = decode_modbus_address(dev_addr)
        self.pmm_addr = get_pmm_addr(self.dev_addr)
        self.pmm = None
        if self.pmm_addr is not None:
            # The PMM is suppressed as part of this device's monitor,
            # so its own handle needs no monitor of its own.
            self.pmm = Modbus(self.pmm_addr, monitor=NullMonitor())

        if monitor is None:
            monitor = RackmonMonitor()
        self.monitor = MonitorChain(monitor, PmmMonitor(self.pmm) if self.pmm else None)

    def read(self, reg, length=1, timeout=0):
        with _translate_errors():
            return pyrmd.RackmonInterface.read(
                self.dev_addr, reg, length=length, timeout=timeout
            )

    def write(self, reg, data, timeout=0):
        with _translate_errors():
            return pyrmd.RackmonInterface.write(
                self.dev_addr, reg, data, timeout=timeout
            )

    def raw(self, req, expected=0, timeout=0):
        wreq = self.addr_b + req
        with _translate_errors():
            resp = pyrmd.RackmonInterface.raw(
                wreq,
                expected,
                timeout=timeout,
                fullResp=False,
                unique_addr=self.unique_addr,
            )
            if resp[0] != self.addr:
                raise ModbusUnknownError()
            return resp[1:]

    def suppress_monitoring(self):
        """
        contextmanager to pause monitoring of this device on entry and
        resume on exit, including exits due to exception
        """
        return self.monitor.suppress()
