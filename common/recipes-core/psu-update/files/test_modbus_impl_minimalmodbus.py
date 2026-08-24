import unittest
from unittest.mock import patch

import modbus_impl_minimalmodbus as backend
from modbus_impl_minimalmodbus import (
    _translate_errors,
    decode_parity,
    DEFAULT_TIMEOUT_MS,
    Modbus,
    MODBUS_RTU_MAX_FRAME,
    ModbusCRCError,
    ModbusException,
    ModbusTimeout,
    ModbusUnknownError,
    PMM_BAUDRATE,
    PMM_PARITY,
    stopbits_for,
)
from modbus_monitor import MonitorChain, NullMonitor, PmmMonitor

# Imported before the modules under test: it puts the fake minimalmodbus
# into sys.modules, which the backend needs to be importable at all.
from test_mocks import minimalmodbus

serial = minimalmodbus.serial


class TestDecodeParity(unittest.TestCase):
    def test_the_single_character_spelling_phosphor_modbus_uses(self):
        self.assertEqual(decode_parity("N"), serial.PARITY_NONE)
        self.assertEqual(decode_parity("E"), serial.PARITY_EVEN)
        self.assertEqual(decode_parity("O"), serial.PARITY_ODD)

    def test_the_long_spelling_a_device_profile_uses(self):
        self.assertEqual(decode_parity("None"), serial.PARITY_NONE)
        self.assertEqual(decode_parity("Even"), serial.PARITY_EVEN)
        self.assertEqual(decode_parity("Odd"), serial.PARITY_ODD)

    def test_case_insensitive(self):
        self.assertEqual(decode_parity("even"), serial.PARITY_EVEN)
        self.assertEqual(decode_parity("ODD"), serial.PARITY_ODD)

    def test_unknown_parity_names_the_ones_it_knows(self):
        with self.assertRaises(ValueError) as ctx:
            decode_parity("Mark")
        self.assertIn("Mark", str(ctx.exception))
        self.assertIn("EVEN", str(ctx.exception))

    def test_a_missing_parity_reads_as_no_parity(self):
        # DeviceConfig.parity_char is None when no device profile
        # provided one, and str(None).upper() lands on "NONE". Pinned
        # rather than endorsed: an unknown parity gives 8N2 instead of
        # an error.
        self.assertEqual(decode_parity(None), serial.PARITY_NONE)


class TestStopbitsFor(unittest.TestCase):
    def test_a_second_stop_bit_takes_the_place_of_the_parity_bit(self):
        # A modbus RTU character is always 11 bits.
        self.assertEqual(stopbits_for(serial.PARITY_NONE), serial.STOPBITS_TWO)

    def test_one_stop_bit_when_there_is_parity(self):
        self.assertEqual(stopbits_for(serial.PARITY_EVEN), serial.STOPBITS_ONE)
        self.assertEqual(stopbits_for(serial.PARITY_ODD), serial.STOPBITS_ONE)


class TestTranslateErrors(unittest.TestCase):
    def test_no_response_is_a_timeout(self):
        with self.assertRaises(ModbusTimeout):
            with _translate_errors():
                raise minimalmodbus.NoResponseError()

    def test_a_malformed_frame_is_reported_as_a_crc_error(self):
        # minimalmodbus does not separate a bad CRC from the other ways
        # a frame can be malformed, and callers only special case the CRC.
        with self.assertRaises(ModbusCRCError):
            with _translate_errors():
                raise minimalmodbus.InvalidResponseError("Checksum error")

    def test_a_local_echo_is_an_unknown_error(self):
        with self.assertRaises(ModbusUnknownError):
            with _translate_errors():
                raise minimalmodbus.LocalEchoError()

    def test_a_slave_error_keeps_the_modbus_error_code(self):
        with self.assertRaises(ModbusException) as ctx:
            with _translate_errors():
                raise minimalmodbus.SlaveReportedException("Illegal data address")
        self.assertIn("Illegal data address", str(ctx.exception))

    def test_other_exceptions_keep_their_own_type(self):
        # Callers rely on ValueError and friends reaching them intact.
        for exc in (ValueError("bad"), KeyError("k"), OSError("gone")):
            with self.subTest(exc=type(exc).__name__):
                with self.assertRaises(type(exc)):
                    with _translate_errors():
                        raise exc


class ModbusTestCase(unittest.TestCase):
    def setUp(self):
        minimalmodbus.Instrument.reset()
        self.addCleanup(minimalmodbus.Instrument.reset)
        # The default monitor talks to phosphor-modbus over D-Bus.
        patcher = patch.object(backend, "PhosphorModbusMonitor")
        self.monitor_cls = patcher.start()
        self.addCleanup(patcher.stop)

    def device(self, addr=0x28, baudrate=19200, parity="E", devpath="/dev/ttyRS485-1"):
        return Modbus(addr, baudrate, parity, devpath, monitor=NullMonitor())


class TestModbusSetup(ModbusTestCase):
    def test_only_the_low_byte_of_the_address_is_significant(self):
        # A handle is already bound to one port, so rackmon's unique
        # addresses have no meaning here.
        dev = self.device(addr=0x0C28)
        self.assertEqual(dev.addr, 0x28)
        self.assertEqual(dev.dev_addr, 0x28)
        self.assertEqual(dev.addr_b, b"\x28")

    def test_line_settings_are_decoded_once(self):
        dev = self.device(parity="Even")
        self.assertEqual(dev.baudrate, 19200)
        self.assertEqual(dev.parity, serial.PARITY_EVEN)
        self.assertEqual(dev.stopbits, serial.STOPBITS_ONE)

    def test_no_parity_means_two_stop_bits(self):
        self.assertEqual(self.device(parity="N").stopbits, serial.STOPBITS_TWO)

    def test_the_instrument_is_opened_on_the_given_port(self):
        dev = self.device(addr=0x28, devpath="/dev/ttyRS485-2")
        self.assertEqual(dev.dev.port, "/dev/ttyRS485-2")
        self.assertEqual(dev.dev.address, 0x28)
        self.assertEqual(dev.dev.mode, minimalmodbus.MODE_RTU)

    def test_phosphor_modbus_is_the_default_monitor(self):
        dev = Modbus(0x28, 19200, "E", "/dev/ttyRS485-1")
        self.monitor_cls.assert_called_once_with("/dev/ttyRS485-1")
        self.assertIs(dev.monitor.monitors[0], self.monitor_cls.return_value)

    def test_a_device_which_is_not_behind_a_pmm(self):
        dev = self.device(addr=0x28)
        self.assertIsNone(dev.pmm)
        self.assertEqual(len(dev.monitor.monitors), 1)

    def test_a_device_behind_a_pmm_suppresses_the_pmm_too(self):
        dev = self.device(addr=0x7B)
        self.assertEqual(dev.pmm_addr, 0x15)
        self.assertEqual(dev.pmm.dev_addr, 0x15)
        self.assertIsInstance(dev.monitor, MonitorChain)
        self.assertIsInstance(dev.monitor.monitors[1], PmmMonitor)

    def test_the_pmm_runs_at_its_own_line_settings_on_the_same_port(self):
        dev = self.device(addr=0x7B, baudrate=19200, parity="N")
        self.assertEqual(dev.pmm.baudrate, PMM_BAUDRATE)
        self.assertEqual(dev.pmm.parity, decode_parity(PMM_PARITY))
        self.assertEqual(dev.pmm.devpath, dev.devpath)

    def test_the_pmm_handle_has_no_monitor_of_its_own(self):
        dev = self.device(addr=0x7B)
        self.assertIsInstance(dev.pmm.monitor.monitors[0], NullMonitor)

    def test_suppress_monitoring_delegates_to_the_chain(self):
        dev = self.device()
        with patch.object(dev.monitor, "suppress") as suppress:
            self.assertIs(dev.suppress_monitoring(), suppress.return_value)


class TestSelect(ModbusTestCase):
    def test_the_settings_are_applied_before_the_transaction(self):
        dev = self.device(baudrate=19200, parity="E")
        with patch.object(dev.dev, "read_registers", return_value=[0]):
            dev.read(0x300, timeout=1000)
        self.assertEqual(dev.dev.serial.baudrate, 19200)
        self.assertEqual(dev.dev.serial.bytesize, serial.EIGHTBITS)
        self.assertEqual(dev.dev.serial.parity, serial.PARITY_EVEN)
        self.assertEqual(dev.dev.serial.stopbits, serial.STOPBITS_ONE)
        self.assertEqual(dev.dev.serial.timeout, 1.0)

    def test_a_zero_timeout_means_the_default_rather_than_no_blocking(self):
        # pyrmd spells "use the default" as timeout=0, pyserial reads it
        # as "never block".
        dev = self.device()
        with patch.object(dev.dev, "read_registers", return_value=[0]):
            dev.read(0x300, timeout=0)
        self.assertEqual(dev.dev.serial.timeout, DEFAULT_TIMEOUT_MS / 1000.0)

    def test_a_device_and_its_pmm_do_not_leave_the_port_on_each_others_settings(self):
        # They share one serial.Serial, so whoever transacts last has to
        # have applied its own settings.
        dev = self.device(addr=0x7B, baudrate=19200, parity="N")
        self.assertIs(dev.dev.serial, dev.pmm.dev.serial)
        with patch.object(dev.pmm.dev, "write_registers"):
            dev.pmm.write(0x7B, 1)
        self.assertEqual(dev.dev.serial.baudrate, PMM_BAUDRATE)
        with patch.object(dev.dev, "read_registers", return_value=[0]):
            dev.read(0x300)
        self.assertEqual(dev.dev.serial.baudrate, 19200)
        self.assertEqual(dev.dev.serial.stopbits, serial.STOPBITS_TWO)


class TestTransfers(ModbusTestCase):
    def setUp(self):
        super().setUp()
        self.dev = self.device(addr=0x28)

    def test_read(self):
        with patch.object(self.dev.dev, "read_registers", return_value=[1, 2]) as read:
            self.assertEqual(self.dev.read(0x300, 2), [1, 2])
        read.assert_called_once_with(0x300, 2)

    def test_read_str(self):
        with patch.object(self.dev.dev, "read_string", return_value="Delta") as read:
            self.assertEqual(self.dev.read_str(8, 8), "Delta")
        read.assert_called_once_with(8, 8)

    def test_write_a_single_register(self):
        # Callers pass a bare value, minimalmodbus only takes a list.
        with patch.object(self.dev.dev, "write_registers") as write:
            self.dev.write(0x300, 0x55AA)
        write.assert_called_once_with(0x300, [0x55AA])

    def test_write_a_block_of_registers(self):
        with patch.object(self.dev.dev, "write_registers") as write:
            self.dev.write(0x310, [1, 2, 3])
        write.assert_called_once_with(0x310, [1, 2, 3])

    def test_write_accepts_any_sequence(self):
        with patch.object(self.dev.dev, "write_registers") as write:
            self.dev.write(0x310, (1, 2))
        write.assert_called_once_with(0x310, [1, 2])

    def test_a_transfer_failure_is_translated(self):
        with patch.object(
            self.dev.dev, "read_registers", side_effect=minimalmodbus.NoResponseError
        ):
            with self.assertRaises(ModbusTimeout):
                self.dev.read(0x300)


class TestRaw(ModbusTestCase):
    def setUp(self):
        super().setUp()
        self.dev = self.device(addr=0x28)

    def response(self, payload, addr=0x28, func=0x43):
        return minimalmodbus._embed_payload(addr, "rtu", func, payload).encode("latin1")

    def test_round_trips_a_frame_and_re_adds_the_function_code(self):
        wire = self.response("\x00\x01")
        with patch.object(self.dev.dev, "_communicate", return_value=wire) as comm:
            resp = self.dev.raw(b"\x43\x02", expected=7)
        sent = comm.call_args[0][0]
        self.assertEqual(sent[:3], b"\x28\x43\x02")
        self.assertEqual(comm.call_args[0][1], 7)
        # The callers' existing checks expect the function code back.
        self.assertEqual(resp, b"\x43\x00\x01")

    def test_reads_a_whole_frame_when_the_size_is_unknown(self):
        wire = self.response("\x00")
        with patch.object(self.dev.dev, "_communicate", return_value=wire) as comm:
            self.dev.raw(b"\x43\x02")
        self.assertEqual(comm.call_args[0][1], MODBUS_RTU_MAX_FRAME)

    def test_a_request_with_no_payload(self):
        wire = self.response("", func=0x42)
        with patch.object(self.dev.dev, "_communicate", return_value=wire):
            self.assertEqual(self.dev.raw(b"\x42"), b"\x42")

    def test_high_bytes_survive_the_latin1_round_trip(self):
        wire = self.response("\xff\xfe")
        with patch.object(self.dev.dev, "_communicate", return_value=wire) as comm:
            resp = self.dev.raw(b"\x43\xff\x00")
        self.assertEqual(comm.call_args[0][0][:4], b"\x28\x43\xff\x00")
        self.assertEqual(resp, b"\x43\xff\xfe")

    def test_a_corrupt_reply_is_a_crc_error(self):
        corrupt = b"\x28\x43\x00\x00"
        with patch.object(self.dev.dev, "_communicate", return_value=corrupt):
            with self.assertRaises(ModbusCRCError):
                self.dev.raw(b"\x43\x02")

    def test_a_reply_from_the_wrong_device_is_rejected(self):
        wire = self.response("\x00", addr=0x29)
        with patch.object(self.dev.dev, "_communicate", return_value=wire):
            with self.assertRaises(ModbusCRCError):
                self.dev.raw(b"\x43\x02")

    def test_a_reply_to_the_wrong_function_is_rejected(self):
        wire = self.response("\x00", func=0x44)
        with patch.object(self.dev.dev, "_communicate", return_value=wire):
            with self.assertRaises(ModbusException):
                self.dev.raw(b"\x43\x02")

    def test_the_line_settings_are_applied_first(self):
        wire = self.response("\x00")
        with patch.object(self.dev.dev, "_communicate", return_value=wire):
            self.dev.raw(b"\x43\x02", timeout=2000)
        self.assertEqual(self.dev.dev.serial.timeout, 2.0)


if __name__ == "__main__":
    unittest.main()
