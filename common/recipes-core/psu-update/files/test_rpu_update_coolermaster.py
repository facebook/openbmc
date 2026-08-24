import io
import os
import struct
import tempfile
import unittest
from unittest.mock import patch

import rpu_update_coolermaster as cm
from rpu_update_coolermaster import (
    BLOCK_SIZE,
    crc32mpeg2,
    file_bincrc,
    file_binlen,
    file_block_end,
    file_block_start,
    file_blocklen,
    file_target,
    parse_image,
    rpu_command,
    rpu_data,
    send_image,
    Status,
    update_rpu,
    upgrade_unlock,
)

# Imported before the modules under test: it puts the fake pyrmd into
# sys.modules, which modbus_update_helper's importers need.
from test_mocks import FakeDevice

UNLOCK_REQ = b"\x64\x01\x19\x00\x01"
ACK = 0xAC


def cmd_reply(cmd, state=ACK, func=0x65):
    return struct.pack(">BBB", func, cmd, state)


def data_reply(block, state=ACK, func=0x66):
    return struct.pack(">BHB", func, block, state)


class QuietTestCase(unittest.TestCase):
    def setUp(self):
        patcher = patch("sys.stdout", new=io.StringIO())
        self.stdout = patcher.start()
        self.addCleanup(patcher.stop)


class TestStatus(unittest.TestCase):
    def test_a_known_code_is_named(self):
        self.assertEqual(str(Status(0xAC)), "ACK")
        self.assertEqual(str(Status(0xBB)), "ERR_BUSY")
        self.assertEqual(str(Status(0xDF)), "NACK_ERR_TCRC")

    def test_an_unknown_code_is_shown_as_hex(self):
        self.assertEqual(str(Status(0x12)), "12")

    def test_check_passes_on_an_ack(self):
        Status(0xAC).check("target")

    def test_check_names_the_command_and_the_failure(self):
        with self.assertRaises(ValueError) as ctx:
            Status(0xE5).check("binlen")
        self.assertIn("binlen", str(ctx.exception))
        self.assertIn("ERR_ENDLEN", str(ctx.exception))


class TestCrc32Mpeg2(unittest.TestCase):
    def test_the_standard_check_value(self):
        self.assertEqual(crc32mpeg2(b"123456789"), 0x0376E6E7)

    def test_an_empty_buffer_is_the_seed(self):
        self.assertEqual(crc32mpeg2(b""), 0xFFFFFFFF)

    def test_the_result_fits_in_32_bits(self):
        self.assertLessEqual(crc32mpeg2(bytes(range(256))), 0xFFFFFFFF)

    def test_it_is_not_the_zlib_crc32(self):
        import zlib

        self.assertNotEqual(crc32mpeg2(b"123456789"), zlib.crc32(b"123456789"))


class TestParseImage(unittest.TestCase):
    def parse(self, content):
        with tempfile.NamedTemporaryFile("wb", delete=False) as f:
            f.write(content)
            path = f.name
        self.addCleanup(os.unlink, path)
        return parse_image(path)

    def test_splits_the_image_into_blocks(self):
        content = bytes(BLOCK_SIZE * 2)
        blocks, size, crc = self.parse(content)
        self.assertEqual(len(blocks), 2)
        self.assertEqual(size, BLOCK_SIZE * 2)
        self.assertEqual(crc, crc32mpeg2(content))

    def test_the_last_block_is_short_rather_than_padded(self):
        blocks, size, _ = self.parse(bytes(BLOCK_SIZE + 5))
        self.assertEqual([len(b) for b in blocks], [BLOCK_SIZE, 5])
        self.assertEqual(size, BLOCK_SIZE + 5)

    def test_an_image_smaller_than_a_block(self):
        blocks, size, _ = self.parse(b"\x01\x02")
        self.assertEqual(blocks, [b"\x01\x02"])
        self.assertEqual(size, 2)

    def test_an_empty_image(self):
        blocks, size, crc = self.parse(b"")
        self.assertEqual(blocks, [])
        self.assertEqual(size, 0)
        self.assertEqual(crc, crc32mpeg2(b""))


class TestCommands(QuietTestCase):
    def test_upgrade_unlock_expects_its_request_echoed(self):
        dev = FakeDevice(raw=[UNLOCK_REQ])
        upgrade_unlock(dev)
        self.assertEqual(dev.calls, [("raw", UNLOCK_REQ, 8, 1000)])

    def test_a_device_which_does_not_unlock(self):
        dev = FakeDevice(raw=[b"\x00"])
        with self.assertRaises(ValueError):
            upgrade_unlock(dev)

    def test_rpu_command_frames_the_command_and_its_payload(self):
        dev = FakeDevice(raw=[cmd_reply(0x02)])
        status = rpu_command(dev, 0x02, b"fw.bin")
        self.assertEqual(dev.calls[0][1], b"\x65\x02\x06fw.bin")
        self.assertEqual(dev.calls[0][2], 6)
        self.assertEqual(str(status), "ACK")

    def test_rpu_command_with_no_payload(self):
        dev = FakeDevice(raw=[cmd_reply(0x4D)])
        rpu_command(dev, 0x4D)
        self.assertEqual(dev.calls[0][1], b"\x65\x4d\x00")

    def test_a_reply_from_another_function(self):
        dev = FakeDevice(raw=[cmd_reply(0x02, func=0x66)])
        with self.assertRaises(ValueError):
            rpu_command(dev, 0x02)

    def test_a_reply_to_another_command(self):
        dev = FakeDevice(raw=[cmd_reply(0x03)])
        with self.assertRaises(ValueError):
            rpu_command(dev, 0x02)

    def test_a_command_the_device_rejects(self):
        dev = FakeDevice(raw=[cmd_reply(0x02, state=0xA1)])
        self.assertEqual(str(rpu_command(dev, 0x02)), "ERR_SYSTEM")

    def test_rpu_data_frames_the_block_number_and_length(self):
        dev = FakeDevice(raw=[data_reply(7)])
        rpu_data(dev, 7, b"\x01\x02")
        self.assertEqual(dev.calls[0][1], b"\x66\x00\x07\x00\x02\x01\x02")
        self.assertEqual(dev.calls[0][2], 7)

    def test_a_reply_about_another_block(self):
        dev = FakeDevice(raw=[data_reply(8)])
        with self.assertRaises(ValueError) as ctx:
            rpu_data(dev, 7, b"\x01")
        self.assertIn("block 7", str(ctx.exception))


class TestFileCommands(QuietTestCase):
    def check(self, fn, cmd, *args):
        dev = FakeDevice(raw=[cmd_reply(cmd)])
        fn(dev, *args)
        return dev.calls[0][1]

    def test_the_target_name_is_sent_as_utf8(self):
        self.assertEqual(self.check(file_target, 0x02, "fw.bin"), b"\x65\x02\x06fw.bin")

    def test_the_lengths_and_the_crc_are_little_endian(self):
        self.assertEqual(
            self.check(file_binlen, 0x04, 0x01020304), b"\x65\x04\x04\x04\x03\x02\x01"
        )
        self.assertEqual(
            self.check(file_bincrc, 0x06, 0xDEADBEEF), b"\x65\x06\x04\xef\xbe\xad\xde"
        )
        self.assertEqual(self.check(file_blocklen, 0x6F, 192), b"\x65\x6f\x02\xc0\x00")
        self.assertEqual(self.check(file_block_start, 0x1A, 3), b"\x65\x1a\x02\x03\x00")

    def test_the_end_marker_has_no_payload(self):
        self.assertEqual(self.check(file_block_end, 0x4D), b"\x65\x4d\x00")

    def test_a_command_the_device_rejects_stops_the_update(self):
        dev = FakeDevice(raw=[cmd_reply(0x04, state=0xE5)])
        with self.assertRaises(ValueError):
            file_binlen(dev, 1)


class TestSendImage(QuietTestCase):
    def test_sends_every_block_in_order(self):
        dev = FakeDevice(raw=[data_reply(i) for i in range(3)])
        send_image(dev, [b"\x01", b"\x02", b"\x03"])
        self.assertEqual(
            [call[1][:3] for call in dev.calls],
            [b"\x66\x00\x00", b"\x66\x00\x01", b"\x66\x00\x02"],
        )

    def test_a_block_the_device_rejects_stops_the_transfer(self):
        dev = FakeDevice(raw=[data_reply(0), data_reply(1, state=0xBB)])
        with self.assertRaises(ValueError) as ctx:
            send_image(dev, [b"\x01", b"\x02", b"\x03"])
        self.assertIn("data-block1", str(ctx.exception))
        self.assertEqual(len(dev.calls), 2)

    def test_an_image_with_no_blocks(self):
        dev = FakeDevice()
        send_image(dev, [])
        self.assertEqual(dev.calls, [])


class TestUpdateRpu(QuietTestCase):
    def test_the_whole_sequence(self):
        blocks = [b"\x01", b"\x02"]
        dev = FakeDevice(
            raw=[
                UNLOCK_REQ,
                cmd_reply(0x02),  # target
                cmd_reply(0x04),  # binlen
                cmd_reply(0x06),  # bincrc
                cmd_reply(0x6F),  # blocklen
                cmd_reply(0x1A),  # blockstart
                data_reply(0),
                data_reply(1),
                cmd_reply(0x4D),  # blockend
            ]
        )
        with patch.object(cm, "parse_image", return_value=(blocks, 2, 0xDEADBEEF)):
            update_rpu(dev, "fw.bin", "fw.bin")
        self.assertEqual(len(dev.calls), 9)
        self.assertEqual(dev.calls[-1][1], b"\x65\x4d\x00")

    def test_the_declared_length_and_crc_come_from_the_image(self):
        dev = FakeDevice(
            raw=[
                UNLOCK_REQ,
                cmd_reply(0x02),
                cmd_reply(0x04),
                cmd_reply(0x06),
                cmd_reply(0x6F),
                cmd_reply(0x1A),
                data_reply(0),
                cmd_reply(0x4D),
            ]
        )
        with patch.object(cm, "parse_image", return_value=([b"\x01"], 1, 0x01020304)):
            update_rpu(dev, "fw.bin")
        self.assertEqual(dev.calls[2][1], b"\x65\x04\x04\x01\x00\x00\x00")
        self.assertEqual(dev.calls[3][1], b"\x65\x06\x04\x04\x03\x02\x01")
        self.assertEqual(dev.calls[4][1][3:], b"\xc0\x00")  # BLOCK_SIZE
        self.assertEqual(dev.calls[5][1][3:], b"\x01\x00")  # one block

    def test_an_rpu_which_does_not_unlock_is_not_written_to(self):
        dev = FakeDevice(raw=[b"\x00"])
        with patch.object(cm, "parse_image", return_value=([b"\x01"], 1, 0)):
            with self.assertRaises(ValueError):
                update_rpu(dev, "fw.bin")
        self.assertEqual(len(dev.calls), 1)


class TestMain(QuietTestCase):
    def test_the_image_is_announced_by_its_basename(self):
        with patch.object(cm, "update_rpu") as update:
            cm.main("dev", "/tmp/some/dir/fw.bin")
        update.assert_called_once_with("dev", "/tmp/some/dir/fw.bin", "fw.bin")

    def test_a_failed_update_exits_non_zero(self):
        with patch.object(cm, "update_rpu", side_effect=ValueError("boom")):
            with patch("sys.stderr", new=io.StringIO()):
                with self.assertRaises(SystemExit) as ctx:
                    cm.main("dev", "fw.bin")
        self.assertEqual(ctx.exception.code, 1)
        self.assertIn("Update Failed", self.stdout.getvalue())


if __name__ == "__main__":
    unittest.main()
