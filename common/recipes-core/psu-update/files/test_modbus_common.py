import unittest

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


class TestExceptions(unittest.TestCase):
    def test_messages_are_the_rackmond_status_strings(self):
        # Callers and logs match on these strings, so they are API.
        self.assertEqual(str(ModbusTimeout()), "ERR_TIMEOUT")
        self.assertEqual(str(ModbusCRCError()), "ERR_BAD_CRC")
        self.assertEqual(str(ModbusUnknownError()), "ERR_IO_FAILURE")
        self.assertEqual(str(ModbusInvalidArgs()), "ERR_INVALID_ARGS")

    def test_all_derive_from_modbus_exception(self):
        for cls in (
            ModbusTimeout,
            ModbusCRCError,
            ModbusUnknownError,
            ModbusInvalidArgs,
        ):
            with self.subTest(cls=cls.__name__):
                self.assertIsInstance(cls(), ModbusException)


class TestDecodeModbusAddress(unittest.TestCase):
    def test_wire_address_has_no_unique_address(self):
        addr, addr_b, uaddr = decode_modbus_address(0x32)
        self.assertEqual(addr, 0x32)
        self.assertEqual(addr_b, b"\x32")
        self.assertIsNone(uaddr)

    def test_unique_address_keeps_low_byte_on_the_wire(self):
        addr, addr_b, uaddr = decode_modbus_address(0x0132)
        self.assertEqual(addr, 0x32)
        self.assertEqual(addr_b, b"\x32")
        self.assertEqual(uaddr, 0x0132)

    def test_0xff_is_not_a_unique_address(self):
        # The boundary: 0xFF still fits on the wire.
        _, _, uaddr = decode_modbus_address(0xFF)
        self.assertIsNone(uaddr)
        _, _, uaddr = decode_modbus_address(0x100)
        self.assertEqual(uaddr, 0x100)

    def test_low_byte_of_a_unique_address_may_be_zero(self):
        addr, addr_b, uaddr = decode_modbus_address(0x0300)
        self.assertEqual(addr, 0)
        self.assertEqual(addr_b, b"\x00")
        self.assertEqual(uaddr, 0x0300)


class TestGetPmmAddr(unittest.TestCase):
    def test_address_inside_a_range(self):
        self.assertEqual(get_pmm_addr(0x30), 0x10)
        self.assertEqual(get_pmm_addr(0x33), 0x10)
        self.assertEqual(get_pmm_addr(0x35), 0x10)

    def test_address_outside_every_range(self):
        self.assertIsNone(get_pmm_addr(0x00))
        self.assertIsNone(get_pmm_addr(0x2F))
        self.assertIsNone(get_pmm_addr(0x39))
        self.assertIsNone(get_pmm_addr(0xFF))

    def test_pmm_keeps_the_upper_bits_of_a_unique_address(self):
        # A PMM is reached on the same rack as the device behind it, so
        # the shelf bits have to survive the lookup.
        self.assertEqual(get_pmm_addr(0x0230), 0x0210)
        self.assertEqual(get_pmm_addr(0x0C7B), 0x0C15)

    def test_ranges_do_not_overlap(self):
        # Every address maps to at most one PMM, which the linear search
        # in get_pmm_addr() silently relies on.
        owners = {}
        for addr in range(0x100):
            pmm = get_pmm_addr(addr)
            if pmm is not None:
                self.assertNotIn(addr, owners)
                owners[addr] = pmm
        self.assertEqual(get_pmm_addr(0x7B), 0x15)

    def test_a_pmm_address_is_not_itself_behind_a_pmm(self):
        for addr in range(0x100):
            pmm = get_pmm_addr(addr)
            if pmm is not None:
                with self.subTest(addr=hex(addr)):
                    self.assertIsNone(get_pmm_addr(pmm))


class TestConstants(unittest.TestCase):
    def test_pmm_pause_register(self):
        self.assertEqual(PMM_PAUSE_REG, 0x7B)


if __name__ == "__main__":
    unittest.main()
