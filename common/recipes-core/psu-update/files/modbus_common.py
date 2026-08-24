"""
Definitions shared by every modbus backend (modbus_impl_*.py).

The exceptions live here rather than in one of the backends so that a
backend does not have to import another backend -- and so drag in that
backend's transport -- just to raise them. Each backend re-exports them,
so callers keep catching them off whichever backend they imported.
"""


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


def decode_modbus_address(dev_addr):
    addr = dev_addr & 0xFF
    addr_b = addr.to_bytes(1, "big")
    uaddr = None if dev_addr <= 0xFF else dev_addr
    return addr, addr_b, uaddr
