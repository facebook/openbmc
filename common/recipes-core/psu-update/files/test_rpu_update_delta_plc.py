import io
import os
import tempfile
import unittest
from unittest.mock import patch

import rpu_update_delta_plc as plc
from rpu_update_delta_plc import (
    Block,
    check_rpu_size,
    fw_upgrade_enabled,
    get_rpu_revision,
    load_fw,
    rpu_stopped,
    syntax_check,
    update_rpu,
    write_block,
    write_fw,
)

# Imported before the modules under test: it puts the fake pyrmd into
# sys.modules, which modbus_update_helper's importers need.
from test_mocks import FakeDevice

STOP_REQ = b"\x05\x0c\x30\x00\x00"
START_REQ = b"\x05\x0c\x30\xff\x00"
ENABLE_REQ = b"\x64\x01\x19\x01\x01"
DISABLE_REQ = b"\x64\x01\x19\x01\x00"
SYNTAX_REQ = b"\x05\x0c\x31\xff\x00"


class QuietTestCase(unittest.TestCase):
    def setUp(self):
        patcher = patch("sys.stdout", new=io.StringIO())
        self.stdout = patcher.start()
        self.addCleanup(patcher.stop)
        patcher = patch("time.sleep")
        self.sleep = patcher.start()
        self.addCleanup(patcher.stop)


class TestBlock(unittest.TestCase):
    def test_an_address_and_a_run_of_words(self):
        block = Block("1000", "00010002ffff")
        self.assertEqual(block.addr, 0x1000)
        self.assertEqual(block.data, [0x0001, 0x0002, 0xFFFF])

    def test_a_trailing_partial_word_is_dropped(self):
        self.assertEqual(Block("0", "0001ab").data, [0x0001])

    def test_a_block_with_no_data(self):
        self.assertEqual(Block("0", "").data, [])

    def test_a_block_longer_than_the_protocol_allows(self):
        with self.assertRaises(ValueError):
            Block("0", "0000" * 0x65)
        self.assertEqual(len(Block("0", "0000" * 0x64).data), 0x64)

    def test_repr_is_the_wire_format_it_came_from(self):
        self.assertEqual(repr(Block("1000", "00010002")), "1000: 0001 0002\n")


class TestLoadFw(unittest.TestCase):
    def load(self, text):
        with tempfile.NamedTemporaryFile("w", delete=False) as f:
            f.write(text)
            path = f.name
        self.addCleanup(os.unlink, path)
        return load_fw(path)

    def test_one_block_per_line(self):
        blocks = self.load("1000:00010002\n2000:0003\n")
        self.assertEqual([b.addr for b in blocks], [0x1000, 0x2000])
        self.assertEqual(blocks[0].data, [1, 2])

    def test_an_empty_file(self):
        self.assertEqual(self.load(""), [])

    def test_a_line_without_a_colon_is_a_bad_image(self):
        with self.assertRaises(IndexError):
            self.load("1000\n")


class TestDeviceQueries(QuietTestCase):
    def test_the_revision_register(self):
        dev = FakeDevice(read_str=["1.0"])
        self.assertEqual(get_rpu_revision(dev), "1.0")
        self.assertEqual(dev.calls, [("read_str", 6632, 4, 0)])

    def test_a_device_which_supports_the_image(self):
        dev = FakeDevice(read=[[0x61A8]])
        check_rpu_size(dev)
        self.assertEqual(dev.calls, [("read", 0x13EA, 0x1, 3000)])

    def test_a_device_which_does_not_support_the_image(self):
        # The image needs a protocol not every device speaks; better to
        # abort before touching the flash.
        dev = FakeDevice(read=[[0x1234]])
        with self.assertRaises(ValueError):
            check_rpu_size(dev)

    def test_a_short_read_is_only_a_warning(self):
        dev = FakeDevice(read=[[]])
        with self.assertRaises(IndexError):
            check_rpu_size(dev)
        self.assertIn("WARNING", self.stdout.getvalue())


class TestRpuStopped(QuietTestCase):
    def test_stops_on_entry_and_starts_on_exit(self):
        dev = FakeDevice(raw=[STOP_REQ, START_REQ])
        with rpu_stopped(dev):
            self.assertEqual(dev.calls, [("raw", STOP_REQ, 8, 0)])
        self.assertEqual(dev.calls[-1], ("raw", START_REQ, 8, 3000))

    def test_a_failed_update_still_starts_the_rpu(self):
        # A stopped RPU is not cooling anything.
        dev = FakeDevice(raw=[STOP_REQ, START_REQ])
        with self.assertRaises(ValueError):
            with rpu_stopped(dev):
                raise ValueError("write failed")
        self.assertEqual(dev.calls[-1][1], START_REQ)

    def test_an_rpu_which_does_not_stop(self):
        dev = FakeDevice(raw=[b"\x00", START_REQ])
        with self.assertRaises(ValueError) as ctx:
            with rpu_stopped(dev):
                self.fail("the update must not run")
        self.assertIn("Bad RPU Stop response", str(ctx.exception))

    def test_an_rpu_which_does_not_start_again_is_reported(self):
        dev = FakeDevice(raw=[STOP_REQ, b"\x00"])
        with self.assertRaises(ValueError) as ctx:
            with rpu_stopped(dev):
                pass
        self.assertIn("Bad RPU Start response", str(ctx.exception))


class TestFwUpgradeEnabled(QuietTestCase):
    def test_enables_on_entry_and_disables_on_exit(self):
        dev = FakeDevice(raw=[ENABLE_REQ, DISABLE_REQ])
        with fw_upgrade_enabled(dev):
            self.assertEqual(dev.calls, [("raw", ENABLE_REQ, 8, 0)])
        self.assertEqual(dev.calls[-1], ("raw", DISABLE_REQ, 8, 0))

    def test_a_failed_update_still_disables_upgrade_mode(self):
        dev = FakeDevice(raw=[ENABLE_REQ, DISABLE_REQ])
        with self.assertRaises(ValueError):
            with fw_upgrade_enabled(dev):
                raise ValueError("write failed")
        self.assertEqual(dev.calls[-1][1], DISABLE_REQ)

    def test_a_device_which_will_not_enter_upgrade_mode(self):
        dev = FakeDevice(raw=[b"\x00", DISABLE_REQ])
        with self.assertRaises(ValueError):
            with fw_upgrade_enabled(dev):
                self.fail("the update must not run")


class TestSyntaxCheck(QuietTestCase):
    def test_accepts_the_echo(self):
        dev = FakeDevice(raw=[SYNTAX_REQ])
        syntax_check(dev)
        self.assertEqual(dev.calls, [("raw", SYNTAX_REQ, 8, 3000)])

    def test_a_program_the_rpu_will_not_accept(self):
        dev = FakeDevice(raw=[b"\x05\x0c\x31\x00\x00"])
        with self.assertRaises(ValueError) as ctx:
            syntax_check(dev)
        self.assertIn("syntax check failed", str(ctx.exception))


class TestWriteBlock(QuietTestCase):
    def block(self):
        return Block("1000", "00010002")

    def test_the_plain_protocol_writes_the_registers(self):
        dev = FakeDevice()
        write_block(dev, self.block(), False)
        self.assertEqual(dev.calls, [("write", 0x1000, [1, 2], 3000)])

    def test_the_oem_protocol_frames_the_block_itself(self):
        dev = FakeDevice(raw=[b"\x75\x10\x00\x00\x02"])
        write_block(dev, self.block(), True)
        (_, req, expected, _) = dev.calls[0]
        self.assertEqual(req, b"\x75\x10\x00\x00\x02\x04\x00\x01\x00\x02")
        self.assertEqual(expected, 2 + 9)

    def test_an_oem_reply_which_does_not_echo_the_block(self):
        dev = FakeDevice(raw=[b"\x75\x20\x00\x00\x02"])
        with self.assertRaises(ValueError) as ctx:
            write_block(dev, self.block(), True)
        self.assertIn("Unexpected response header", str(ctx.exception))

    def test_write_fw_sends_every_block(self):
        blocks = [Block("1000", "0001"), Block("1001", "0002")]
        dev = FakeDevice()
        write_fw(dev, blocks, False)
        self.assertEqual(
            dev.calls,
            [("write", 0x1000, [1], 3000), ("write", 0x1001, [2], 3000)],
        )


class TestUpdateRpu(QuietTestCase):
    def test_the_whole_sequence(self):
        blocks = [Block("1000", "0001")]
        dev = FakeDevice(
            read_str=["1.0", "2.0"],
            raw=[STOP_REQ, ENABLE_REQ, DISABLE_REQ, SYNTAX_REQ, START_REQ],
        )
        with patch.object(plc, "load_fw", return_value=blocks):
            update_rpu(dev, "fw.txt", False)
        self.assertEqual(
            [call[1] for call in dev.calls_of("raw")],
            [STOP_REQ, ENABLE_REQ, DISABLE_REQ, SYNTAX_REQ, START_REQ],
        )
        self.assertEqual(dev.calls_of("write"), [("write", 0x1000, [1], 3000)])

    def test_the_oem_protocol_checks_the_device_before_stopping_it(self):
        dev = FakeDevice(read_str=["1.0"], read=[[0x1234]])
        with patch.object(plc, "load_fw", return_value=[]):
            with self.assertRaises(ValueError):
                update_rpu(dev, "fw.txt", True)
        # Never stopped the RPU.
        self.assertEqual(dev.calls_of("raw"), [])

    def test_the_plain_protocol_does_not_check_the_device(self):
        dev = FakeDevice(
            read_str=["1.0", "2.0"],
            raw=[STOP_REQ, ENABLE_REQ, DISABLE_REQ, SYNTAX_REQ, START_REQ],
        )
        with patch.object(plc, "load_fw", return_value=[]):
            update_rpu(dev, "fw.txt", False)
        self.assertEqual(dev.calls_of("read"), [])


class TestMain(QuietTestCase):
    def test_a_successful_update_returns(self):
        with patch.object(plc, "update_rpu") as update:
            plc.main("dev", "fw.txt", True)
        update.assert_called_once_with("dev", "fw.txt", True)

    def test_a_failed_update_exits_non_zero(self):
        with patch.object(plc, "update_rpu", side_effect=ValueError("boom")):
            with patch("sys.stderr", new=io.StringIO()):
                with self.assertRaises(SystemExit) as ctx:
                    plc.main("dev", "fw.txt", True)
        self.assertEqual(ctx.exception.code, 1)
        self.assertIn("Update Failed", self.stdout.getvalue())


if __name__ == "__main__":
    unittest.main()
