from contextlib import contextmanager

import pyrmd
from modbus_common import (
    decode_modbus_address,
    ModbusCRCError,
    ModbusException,
    ModbusInvalidArgs,
    ModbusTimeout,
    ModbusUnknownError,
)

# Re-exported so users of this module keep catching the exceptions off
# the backend they imported. Both backends raise the same classes.
__all__ = [
    "ModbusCRCError",
    "ModbusException",
    "ModbusInvalidArgs",
    "ModbusTimeout",
    "ModbusUnknownError",
    "Modbus",
    "decode_modbus_address",
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
    """

    def __init__(self, dev_addr):
        self.dev_addr = dev_addr
        self.addr, self.addr_b, self.unique_addr = decode_modbus_address(dev_addr)

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
