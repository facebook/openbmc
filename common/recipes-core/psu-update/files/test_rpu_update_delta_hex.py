import io
import os
import tempfile
import unittest
from unittest.mock import patch

import rpu_update_delta_hex as hexfw
from rpu_update_delta_hex import (
    check_update_result,
    HEX_UPDATE_RESULT_REG,
    IMAGE_SIZE_WR_REG,
    IMAGE_WR_REG,
    load_file,
    NOTIFY_HEX_IMAGE_SIZE_REG,
    NOTIFY_HEX_IMAGE_WR_REG,
    OPERATING_MODE_AP,
    OPERATING_MODE_BOOT,
    OPERATING_MODE_RD_REG,
    OPERATING_MODE_WR_REG,
    send_image,
    update_hex,
    verify_operating_mode,
    verify_update,
    write_data,
    write_fw_img_len,
    write_operating_mode,
    write_transmission_complete,
)

# Imported before the modules under test: it puts the fake pyrmd into
# sys.modules, which modbus_update_helper's importers need.
from test_mocks import FakeDevice


class QuietTestCase(unittest.TestCase):
    def setUp(self):
        patcher = patch("sys.stdout", new=io.StringIO())
        self.stdout = patcher.start()
        self.addCleanup(patcher.stop)
        patcher = patch("time.sleep")
        self.sleep = patcher.start()
        self.addCleanup(patcher.stop)


class TestLoadFile(unittest.TestCase):
    def load(self, content):
        with tempfile.NamedTemporaryFile("wb", delete=False) as f:
            f.write(content)
            path = f.name
        self.addCleanup(os.unlink, path)
        return load_file(path)

    def test_bytes_become_big_endian_register_values(self):
        self.assertEqual(self.load(b"\x12\x34\xab\xcd"), [0x1234, 0xABCD])

    def test_an_odd_trailing_byte_is_dropped(self):
        self.assertEqual(self.load(b"\x12\x34\x56"), [0x1234])


class TestOperatingMode(QuietTestCase):
    def test_writing_the_mode(self):
        dev = FakeDevice()
        write_operating_mode(dev, OPERATING_MODE_BOOT)
        self.assertEqual(
            dev.calls, [("write", OPERATING_MODE_WR_REG, OPERATING_MODE_BOOT, 0)]
        )
        # The PLC needs a moment to relay it to the HEX.
        self.sleep.assert_called_once_with(1.0)

    def test_the_mode_is_read_back_from_a_different_register(self):
        dev = FakeDevice(read=[[OPERATING_MODE_BOOT]])
        verify_operating_mode(dev, OPERATING_MODE_BOOT)
        self.assertEqual(dev.calls, [("read", OPERATING_MODE_RD_REG, 1, 0)])

    def test_a_device_which_did_not_change_mode(self):
        dev = FakeDevice(read=[[OPERATING_MODE_AP]])
        with self.assertRaises(ValueError) as ctx:
            verify_operating_mode(dev, OPERATING_MODE_BOOT)
        self.assertIn("Curr mode 1 != expected 0", str(ctx.exception))


class TestWriteFwImgLen(QuietTestCase):
    def test_the_length_is_in_bytes_and_big_endian(self):
        dev = FakeDevice()
        write_fw_img_len(dev, [0] * 0x8000)  # 0x10000 bytes
        self.assertEqual(
            dev.calls,
            [
                ("write", IMAGE_SIZE_WR_REG, [0x0001, 0x0000], 0),
                ("write", NOTIFY_HEX_IMAGE_SIZE_REG, 0xFF00, 0),
            ],
        )

    def test_a_short_image(self):
        dev = FakeDevice()
        write_fw_img_len(dev, [0] * 3)
        self.assertEqual(dev.calls[0][2], [0x0000, 0x0006])


class TestWriteData(QuietTestCase):
    def test_a_full_block_is_written_and_notified(self):
        dev = FakeDevice(read=[[1]])
        write_data(dev, [1, 2, 3, 4], 4)
        self.assertEqual(
            [(call[0], call[1], call[2]) for call in dev.calls_of("write")],
            [
                ("write", IMAGE_WR_REG, [1, 2, 3, 4]),
                ("write", NOTIFY_HEX_IMAGE_WR_REG, 0xFF00),
            ],
        )

    def test_a_short_block_is_padded_with_erased_flash(self):
        dev = FakeDevice(read=[[1]])
        write_data(dev, [1, 2], 4)
        self.assertEqual(dev.calls_of("write")[0][2], [1, 2, 0xFFFF, 0xFFFF])

    def test_the_plc_is_given_time_to_reach_the_hex(self):
        dev = FakeDevice(read=[[1]])
        write_data(dev, [1], 1)
        self.sleep.assert_called_once_with(1.0)

    def test_the_transfer_is_confirmed_before_the_next_block(self):
        dev = FakeDevice(read=[[0], [0], [1]])
        write_data(dev, [1], 1)
        self.assertEqual(len(dev.calls_of("read")), 3)

    def test_a_block_the_hex_never_acknowledges(self):
        dev = FakeDevice(read=[[0]] * 11)
        with self.assertRaises(ValueError):
            write_data(dev, [1], 1)

    def test_write_transmission_complete_checks_the_status_register(self):
        dev = FakeDevice(read=[[1]])
        write_transmission_complete(dev)
        self.assertEqual(dev.calls, [("read", hexfw.HEX_TRANSMISSION_STATUS_REG, 1, 0)])


class TestSendImage(QuietTestCase):
    def test_sends_the_image_in_96_word_blocks(self):
        dev = FakeDevice(read=[[1]] * 2)
        send_image(dev, list(range(192)))
        blocks = [call[2] for call in dev.calls_of("write") if call[1] == IMAGE_WR_REG]
        self.assertEqual(len(blocks), 2)
        self.assertEqual(blocks[0], list(range(96)))

    def test_a_partial_block_at_the_end_is_padded_and_sent(self):
        dev = FakeDevice(read=[[1]] * 2)
        send_image(dev, list(range(100)))
        blocks = [call[2] for call in dev.calls_of("write") if call[1] == IMAGE_WR_REG]
        self.assertEqual(len(blocks), 2)
        self.assertEqual(blocks[1], list(range(96, 100)) + [0xFFFF] * 92)

    def test_an_empty_image_sends_nothing(self):
        dev = FakeDevice()
        send_image(dev, [])
        self.assertEqual(dev.calls, [])


class TestVerifyUpdate(QuietTestCase):
    def test_reports_the_transfer_complete_and_reads_the_result(self):
        dev = FakeDevice(read=[[0]])
        verify_update(dev)
        self.assertEqual(
            dev.calls,
            [
                ("write", hexfw.FW_TRANSMISSION_STATUS_REG, 1, 0),
                ("read", HEX_UPDATE_RESULT_REG, 1, 0),
            ],
        )

    def test_a_device_which_reports_a_failure(self):
        dev = FakeDevice(read=[[3]] * 6)
        with self.assertRaises(ValueError) as ctx:
            check_update_result(dev)
        self.assertIn("Update failed: 3", str(ctx.exception))

    def test_a_result_which_only_settles_after_a_retry(self):
        dev = FakeDevice(read=[[3], [0]])
        check_update_result(dev)
        self.assertEqual(len(dev.calls), 2)


class TestUpdateHex(QuietTestCase):
    def test_the_whole_sequence(self):
        dev = FakeDevice(
            read=[
                [OPERATING_MODE_BOOT],  # verify_operating_mode
                [1],  # transmission complete
                [0],  # update result
            ]
        )
        with patch.object(hexfw, "load_file", return_value=list(range(96))):
            update_hex(dev, "fw.bin")
        self.assertEqual(
            [call[1] for call in dev.calls_of("write")],
            [
                OPERATING_MODE_WR_REG,
                IMAGE_SIZE_WR_REG,
                NOTIFY_HEX_IMAGE_SIZE_REG,
                IMAGE_WR_REG,
                NOTIFY_HEX_IMAGE_WR_REG,
                hexfw.FW_TRANSMISSION_STATUS_REG,
            ],
        )

    def test_a_device_which_will_not_enter_boot_mode_is_not_written_to(self):
        dev = FakeDevice(read=[[OPERATING_MODE_AP]])
        with patch.object(hexfw, "load_file", return_value=[0]):
            with self.assertRaises(ValueError):
                update_hex(dev, "fw.bin")
        self.assertEqual(len(dev.calls_of("write")), 1)


class TestMain(QuietTestCase):
    def test_a_successful_update_says_so(self):
        with patch.object(hexfw, "update_hex") as update:
            hexfw.main("dev", "fw.bin")
        update.assert_called_once_with("dev", "fw.bin")
        self.assertIn("Update Successful!", self.stdout.getvalue())

    def test_a_failed_update_exits_non_zero(self):
        with patch.object(hexfw, "update_hex", side_effect=ValueError("boom")):
            with patch("sys.stderr", new=io.StringIO()):
                with self.assertRaises(SystemExit) as ctx:
                    hexfw.main("dev", "fw.bin")
        self.assertEqual(ctx.exception.code, 1)
        self.assertNotIn("Update Successful!", self.stdout.getvalue())


if __name__ == "__main__":
    unittest.main()
