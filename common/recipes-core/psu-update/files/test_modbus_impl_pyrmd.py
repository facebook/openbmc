import unittest
from unittest.mock import patch

from modbus_impl_pyrmd import (
    _translate_errors,
    Modbus,
    ModbusCRCError,
    ModbusException,
    ModbusInvalidArgs,
    ModbusTimeout,
    ModbusUnknownError,
)
from modbus_monitor import MonitorChain, NullMonitor, PmmMonitor, RackmonMonitor

# Imported before the modules under test: it puts the fake pyrmd into
# sys.modules, which modbus_impl_pyrmd needs to be importable at all.
from test_mocks import pyrmd


class TestTranslateErrors(unittest.TestCase):
    def test_each_pyrmd_exception_becomes_the_backend_independent_one(self):
        cases = [
            (pyrmd.ModbusTimeout, ModbusTimeout),
            (pyrmd.ModbusCRCError, ModbusCRCError),
            (pyrmd.ModbusUnknownError, ModbusUnknownError),
            (pyrmd.ModbusInvalidArgs, ModbusInvalidArgs),
        ]
        for raised, expected in cases:
            with self.subTest(raised=raised.__name__):
                with self.assertRaises(expected) as ctx:
                    with _translate_errors():
                        raise raised()
                # The subclasses have to be caught before their base.
                self.assertIs(type(ctx.exception), expected)
                self.assertIsInstance(ctx.exception.__cause__, raised)

    def test_an_unclassified_failure_keeps_the_rackmond_status(self):
        with self.assertRaises(ModbusException) as ctx:
            with _translate_errors():
                raise pyrmd.ModbusException("ERR_FLASH_BUSY")
        self.assertEqual(str(ctx.exception), "ERR_FLASH_BUSY")

    def test_other_exceptions_pass_through(self):
        with self.assertRaises(ValueError):
            with _translate_errors():
                raise ValueError("not a modbus problem")

    def test_success_is_not_disturbed(self):
        with _translate_errors():
            pass


class TestModbusAddressing(unittest.TestCase):
    def test_a_wire_address(self):
        dev = Modbus(0x32, monitor=NullMonitor())
        self.assertEqual(dev.dev_addr, 0x32)
        self.assertEqual(dev.addr, 0x32)
        self.assertEqual(dev.addr_b, b"\x32")
        self.assertIsNone(dev.unique_addr)

    def test_a_unique_address_keeps_both_forms(self):
        dev = Modbus(0x0132, monitor=NullMonitor())
        self.assertEqual(dev.dev_addr, 0x0132)
        self.assertEqual(dev.addr, 0x32)
        self.assertEqual(dev.unique_addr, 0x0132)


class TestModbusMonitors(unittest.TestCase):
    def test_rackmon_is_the_default_monitor(self):
        dev = Modbus(0x28)
        self.assertIsInstance(dev.monitor, MonitorChain)
        self.assertEqual(len(dev.monitor.monitors), 1)
        self.assertIsInstance(dev.monitor.monitors[0], RackmonMonitor)

    def test_a_device_which_is_not_behind_a_pmm(self):
        # 0x28 is outside every PMM's range.
        dev = Modbus(0x28, monitor=NullMonitor())
        self.assertIsNone(dev.pmm_addr)
        self.assertIsNone(dev.pmm)

    def test_a_device_behind_a_pmm_suppresses_the_pmm_too(self):
        dev = Modbus(0x7B, monitor=NullMonitor())
        self.assertEqual(dev.pmm_addr, 0x15)
        self.assertEqual(dev.pmm.dev_addr, 0x15)
        self.assertEqual(len(dev.monitor.monitors), 2)
        self.assertIsInstance(dev.monitor.monitors[1], PmmMonitor)
        self.assertIs(dev.monitor.monitors[1].pmm, dev.pmm)

    def test_the_pmm_handle_has_no_monitor_of_its_own(self):
        # It is suppressed as part of the device's chain; monitoring it
        # separately would pause and resume the same thing twice.
        dev = Modbus(0x7B, monitor=NullMonitor())
        self.assertEqual(len(dev.pmm.monitor.monitors), 1)
        self.assertIsInstance(dev.pmm.monitor.monitors[0], NullMonitor)

    def test_the_pmm_of_a_unique_address_is_on_the_same_shelf(self):
        dev = Modbus(0x0C7B, monitor=NullMonitor())
        self.assertEqual(dev.pmm.dev_addr, 0x0C15)

    def test_suppress_monitoring_delegates_to_the_chain(self):
        dev = Modbus(0x32, monitor=NullMonitor())
        with patch.object(dev.monitor, "suppress") as suppress:
            self.assertIs(dev.suppress_monitoring(), suppress.return_value)


class TestModbusTransfers(unittest.TestCase):
    def setUp(self):
        self.dev = Modbus(0x0132, monitor=NullMonitor())

    def test_read(self):
        with patch.object(pyrmd.RackmonInterface, "read", return_value=[1, 2]) as read:
            self.assertEqual(self.dev.read(0x300, length=2, timeout=1000), [1, 2])
        read.assert_called_once_with(0x0132, 0x300, length=2, timeout=1000)

    def test_read_defaults(self):
        with patch.object(pyrmd.RackmonInterface, "read", return_value=[1]) as read:
            self.dev.read(0x300)
        read.assert_called_once_with(0x0132, 0x300, length=1, timeout=0)

    def test_read_str_asks_rackmon_for_the_decoded_register(self):
        # rackmon knows the register map, so the length is its business.
        with patch.object(pyrmd.RackmonInterface, "get", return_value="Delta") as get:
            self.assertEqual(self.dev.read_str(8, 8), "Delta")
        get.assert_called_once_with(0x0132, 8, True)

    def test_write(self):
        with patch.object(pyrmd.RackmonInterface, "write") as write:
            self.dev.write(0x300, 0x55AA, timeout=1000)
        write.assert_called_once_with(0x0132, 0x300, 0x55AA, timeout=1000)

    def test_write_a_block_of_registers(self):
        with patch.object(pyrmd.RackmonInterface, "write") as write:
            self.dev.write(0x310, [1, 2, 3])
        write.assert_called_once_with(0x0132, 0x310, [1, 2, 3], timeout=0)

    def test_a_transfer_failure_is_translated(self):
        with patch.object(
            pyrmd.RackmonInterface, "read", side_effect=pyrmd.ModbusTimeout
        ):
            with self.assertRaises(ModbusTimeout):
                self.dev.read(0x300)

    def test_raw_frames_the_request_and_strips_the_address(self):
        with patch.object(
            pyrmd.RackmonInterface, "raw", return_value=b"\x32\x43\x02\x00\x01"
        ) as raw:
            resp = self.dev.raw(b"\x43\x02", expected=7, timeout=2000)
        raw.assert_called_once_with(
            b"\x32\x43\x02", 7, timeout=2000, fullResp=False, unique_addr=0x0132
        )
        self.assertEqual(resp, b"\x43\x02\x00\x01")

    def test_raw_on_a_wire_address_has_no_unique_address(self):
        dev = Modbus(0x32, monitor=NullMonitor())
        with patch.object(
            pyrmd.RackmonInterface, "raw", return_value=b"\x32\x43"
        ) as raw:
            dev.raw(b"\x43")
        self.assertIsNone(raw.call_args[1]["unique_addr"])

    def test_a_reply_from_the_wrong_device_is_an_error(self):
        with patch.object(pyrmd.RackmonInterface, "raw", return_value=b"\x33\x43\x02"):
            with self.assertRaises(ModbusUnknownError):
                self.dev.raw(b"\x43\x02")


if __name__ == "__main__":
    unittest.main()
