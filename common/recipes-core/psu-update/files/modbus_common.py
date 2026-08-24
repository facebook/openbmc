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


# Register which pauses a PMM's monitoring of the devices behind it.
PMM_PAUSE_REG = 0x7B


def decode_modbus_address(dev_addr):
    addr = dev_addr & 0xFF
    addr_b = addr.to_bytes(1, "big")
    uaddr = None if dev_addr <= 0xFF else dev_addr
    return addr, addr_b, uaddr


def get_pmm_addr(dev_addr):
    addr = dev_addr & 0xFF
    upper = dev_addr & 0xFF00
    pmm_addrs = {
        0x10: [[0x30, 0x35]],
        0x11: [[0x3A, 0x3F]],
        0x12: [[0x5A, 0x5F]],
        0x13: [[0x6A, 0x6F]],
        0x14: [[0x70, 0x75]],
        0x15: [[0x7A, 0x7F]],
        0x16: [[0x80, 0x85]],
        0x17: [[0x40, 0x45]],
        0x20: [[0x90, 0x95]],
        0x21: [[0x9A, 0x9F]],
        0x22: [[0xAA, 0xAF]],
        0x23: [[0xBA, 0xBF]],
        0x24: [[0xD0, 0xD5]],
        0x25: [[0xDA, 0xDF]],
        0x26: [[0xA0, 0xA5]],
        0x27: [[0xB0, 0xB5]],
        0xF0: [[0x36, 0x38]],
        0xF1: [[0x46, 0x48]],
        0xF2: [[0x56, 0x58]],
        0xF3: [[0x66, 0x68]],
        0xF4: [[0x76, 0x78]],
        0xF5: [[0x86, 0x88]],
        0xF6: [[0x96, 0x98]],
        0xF7: [[0xA6, 0xA8]],
    }
    for pmm_addr, addr_ranges in pmm_addrs.items():
        for addr_range in addr_ranges:
            if addr >= addr_range[0] and addr <= addr_range[1]:
                # Return a unique address for the PMM by adding back
                # the upper bits of the original address.
                return pmm_addr | upper
    return None
