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
    get_rpu_revision,
    parse_file_path,
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


class TestGetRpuRevision(QuietTestCase):
    def test_it_reads_the_components_version_register(self):
        dev = FakeDevice(read_str=["1.2.3"])
        self.assertEqual(get_rpu_revision(dev, "FAN_RACK_1_ETH"), "1.2.3")
        self.assertEqual(dev.calls, [("read_str", 0x1AB, 4, 0)])

    def test_every_component_names_a_register_and_an_image(self):
        for comp, info in cm.AALCV2_COMPONENTS.items():
            with self.subTest(component=comp):
                reg, num = info["vers_reg"]
                self.assertIsInstance(reg, int)
                self.assertIn(num, (4, 5))
                self.assertTrue(info["name"].endswith(".tar.gz"))

    def test_no_two_components_share_an_image_name(self):
        names = [info["name"] for info in cm.AALCV2_COMPONENTS.values()]
        self.assertEqual(len(set(names)), len(names))


class TestParseFilePath(unittest.TestCase):
    def test_the_component_comes_from_the_name(self):
        self.assertEqual(
            parse_file_path("/tmp/some/dir/MT-E_F1_1.2.3.tar.gz"), "FAN_RACK_1_ETH"
        )
        self.assertEqual(parse_file_path("UPSPFC_P_9.9.tar.gz"), "PUMP_RACK_UPSPFC")

    def test_a_version_may_contain_underscores(self):
        self.assertEqual(parse_file_path("MT-R_F2_1_0_5.tar.gz"), "FAN_RACK_2_RPU")

    def test_a_name_which_is_not_a_tarball(self):
        with self.assertRaises(ValueError) as ctx:
            parse_file_path("MT-E_F1_1.2.3.bin")
        self.assertIn("expected tar.gz", str(ctx.exception))

    def test_a_name_which_is_missing_a_field(self):
        with self.assertRaises(ValueError) as ctx:
            parse_file_path("MT-E_F1.tar.gz")
        self.assertIn("3 parts", str(ctx.exception))

    def test_a_name_naming_no_component_we_know(self):
        with self.assertRaises(ValueError) as ctx:
            parse_file_path("MT-X_F9_1.0.tar.gz")
        self.assertIn("Unknown component MT-X or target F9", str(ctx.exception))

    def test_the_suffix_is_only_stripped_from_the_end(self):
        # A stray ".tar.gz" inside the name must not be swallowed.
        with self.assertRaises(ValueError) as ctx:
            parse_file_path("MT-E.tar.gz_F1_1.0.tar.gz")
        self.assertIn("Unknown component", str(ctx.exception))

    def test_every_image_name_round_trips_to_its_component(self):
        for comp, info in cm.AALCV2_COMPONENTS.items():
            with self.subTest(component=comp):
                name = info["name"].replace(".tar.gz", "_1.0.tar.gz")
                self.assertEqual(parse_file_path(name), comp)


class TestUpdateRpu(QuietTestCase):
    def setUp(self):
        super().setUp()
        patcher = patch.object(cm.time, "sleep")
        self.sleep = patcher.start()
        self.addCleanup(patcher.stop)

    def full_exchange(self, num_blocks):
        return (
            [
                UNLOCK_REQ,
                cmd_reply(0x02),  # target
                cmd_reply(0x04),  # binlen
                cmd_reply(0x06),  # bincrc
                cmd_reply(0x6F),  # blocklen
                cmd_reply(0x1A),  # blockstart
            ]
            + [data_reply(i) for i in range(num_blocks)]
            + [cmd_reply(0x4D)]  # blockend
        )

    def test_the_whole_sequence(self):
        blocks = [b"\x01", b"\x02"]
        dev = FakeDevice(raw=self.full_exchange(2), read_str=["1.0.0", "1.0.1"])
        with patch.object(cm, "parse_image", return_value=(blocks, 2, 0xDEADBEEF)):
            update_rpu(dev, "fw.bin", "PUMP_RACK_RPU")
        raws = dev.calls_of("raw")
        self.assertEqual(len(raws), 9)
        self.assertEqual(raws[-1][1], b"\x65\x4d\x00")

    def test_the_device_is_told_the_components_canonical_image_name(self):
        # Not the path it was read from: the RPU keys off this name.
        dev = FakeDevice(raw=self.full_exchange(1), read_str=["1.0.0", "1.0.1"])
        with patch.object(cm, "parse_image", return_value=([b"\x01"], 1, 0)):
            update_rpu(dev, "/tmp/MT-R_P_1.0.1.tar.gz", "PUMP_RACK_RPU")
        self.assertEqual(dev.calls_of("raw")[1][1], b"\x65\x02\x0dMT-R_P.tar.gz")

    def test_the_version_is_read_before_and_after_the_update(self):
        dev = FakeDevice(raw=self.full_exchange(1), read_str=["1.0.0", "1.0.1"])
        with patch.object(cm, "parse_image", return_value=([b"\x01"], 1, 0)):
            update_rpu(dev, "fw.bin", "FAN_RACK_2_UPSCOM")
        self.assertEqual(
            dev.calls_of("read_str"),
            [("read_str", 0x9174, 5, 0), ("read_str", 0x9174, 5, 0)],
        )
        self.assertIn("Current Version: 1.0.0", self.stdout.getvalue())
        self.assertIn("Version After Upgrade: 1.0.1", self.stdout.getvalue())

    def test_the_component_is_given_time_to_reboot_before_it_is_re_read(self):
        dev = FakeDevice(raw=self.full_exchange(1), read_str=["1.0.0", "1.0.1"])
        with patch.object(cm, "parse_image", return_value=([b"\x01"], 1, 0)):
            update_rpu(dev, "fw.bin", "PUMP_RACK_RPU")
        self.sleep.assert_called_once_with(cm.REBOOT_SECS)
        # The last thing on the wire is the read-back, so the sleep landed
        # between the end of the transfer and it.
        self.assertEqual(dev.calls[-1][0], "read_str")

    def test_the_declared_length_and_crc_come_from_the_image(self):
        dev = FakeDevice(raw=self.full_exchange(1), read_str=["1.0.0", "1.0.1"])
        with patch.object(cm, "parse_image", return_value=([b"\x01"], 1, 0x01020304)):
            update_rpu(dev, "fw.bin", "PUMP_RACK_RPU")
        raws = dev.calls_of("raw")
        self.assertEqual(raws[2][1], b"\x65\x04\x04\x01\x00\x00\x00")
        self.assertEqual(raws[3][1], b"\x65\x06\x04\x04\x03\x02\x01")
        self.assertEqual(raws[4][1][3:], b"\xc0\x00")  # BLOCK_SIZE
        self.assertEqual(raws[5][1][3:], b"\x01\x00")  # one block

    def test_an_rpu_which_does_not_unlock_is_not_written_to(self):
        dev = FakeDevice(raw=[b"\x00"], read_str=["1.0.0"])
        with patch.object(cm, "parse_image", return_value=([b"\x01"], 1, 0)):
            with self.assertRaises(ValueError):
                update_rpu(dev, "fw.bin", "PUMP_RACK_RPU")
        self.assertEqual(len(dev.calls_of("raw")), 1)

    def test_an_unknown_component_is_rejected_before_the_device_is_touched(self):
        dev = FakeDevice()
        with self.assertRaises(KeyError):
            update_rpu(dev, "fw.bin", "NO_SUCH_COMPONENT")
        self.assertEqual(dev.calls, [])


class TestMain(QuietTestCase):
    def test_the_component_is_derived_from_the_file_name(self):
        path = "/tmp/some/dir/MT-E_F1_1.2.3.tar.gz"
        with patch.object(cm, "update_rpu") as update:
            cm.main("dev", path)
        update.assert_called_once_with("dev", path, "FAN_RACK_1_ETH")

    def test_an_explicit_component_overrides_the_file_name(self):
        with patch.object(cm, "update_rpu") as update:
            cm.main("dev", "fw.bin", "PUMP_RACK_RPU")
        update.assert_called_once_with("dev", "fw.bin", "PUMP_RACK_RPU")

    def test_a_failed_update_exits_non_zero(self):
        with patch.object(cm, "update_rpu", side_effect=ValueError("boom")):
            with patch("sys.stderr", new=io.StringIO()):
                with self.assertRaises(SystemExit) as ctx:
                    cm.main("dev", "fw.bin", "PUMP_RACK_RPU")
        self.assertEqual(ctx.exception.code, 1)
        self.assertIn("Update Failed", self.stdout.getvalue())


if __name__ == "__main__":
    unittest.main()
