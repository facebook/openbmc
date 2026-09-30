import io
import struct
import unittest
from unittest.mock import patch

import psu_update_aei
from modbus_impl_pyrmd import ModbusTimeout
from psu_update_aei import (
    BadAEIResponse,
    device_params,
    isp,
    isp_enter,
    isp_exit,
    isp_flash_block,
    isp_get_status,
    StatusRegister,
    transfer_image,
    update_device,
    wait_update_complete,
)

# Imported before the modules under test: it puts the fake pyrmd into
# sys.modules, which psu_update_aei needs to be importable at all.
from test_mocks import FakeDevice


def status_reply(value):
    return struct.pack(">BBH", 0x43, 2, value)


def enter_reply(cmd=0x1):
    return struct.pack(">BBB", 0x42, cmd, 0)


def exit_reply(status=0, cmd=0x0):
    return struct.pack(">BBBB", 0x42, cmd, status, 0)


def block_reply(block_no, code=0x0, func=0x45, length=0x3):
    return struct.pack(">BBHB", func, length, block_no, code)


class QuietTestCase(unittest.TestCase):
    """Silences progress output and makes the sleeps free"""

    def setUp(self):
        patcher = patch("sys.stdout", new=io.StringIO())
        self.stdout = patcher.start()
        self.addCleanup(patcher.stop)
        # Both the updater and the retry decorator sleep on the way
        # through; one patch covers both.
        patcher = patch("time.sleep")
        self.sleep = patcher.start()
        self.addCleanup(patcher.stop)


class TestStatusRegister(unittest.TestCase):
    def test_a_field_is_a_bit(self):
        status = StatusRegister(0x1)
        self.assertTrue(status["FULL_IMAGE_RECEIVED"])
        self.assertFalse(status["FULL_IMAGE_RECV_PENDING"])

    def test_the_second_byte_carries_the_checksum_results(self):
        status = StatusRegister(1 << 8)
        self.assertTrue(status["PRIMARY_DSP_CHECKSUM_FAILED"])

    def test_get_set_names_the_bits_which_are_set(self):
        value = (1 << 0) | (1 << 8) | (1 << 14)
        self.assertEqual(
            StatusRegister(value).getSet(),
            {
                "FULL_IMAGE_RECEIVED",
                "PRIMARY_DSP_CHECKSUM_FAILED",
                "PRIMARY_DSP_UPDATE_FAILED",
            },
        )

    def test_get_set_leaves_out_the_reserved_bits(self):
        self.assertEqual(StatusRegister(0xFF).getSet() & {"RESERVED"}, set())
        self.assertEqual(StatusRegister(0).getSet(), set())

    def test_str_lists_every_field(self):
        text = str(StatusRegister(0x1))
        self.assertIn("('FULL_IMAGE_RECEIVED', True)", text)
        self.assertIn("('FULL_IMAGE_RECV_PENDING', False)", text)

    def test_an_unknown_field_name(self):
        with self.assertRaises(ValueError):
            StatusRegister(0)["NO_SUCH_BIT"]


class TestIspGetStatus(QuietTestCase):
    def test_asks_for_the_status_register(self):
        dev = FakeDevice(raw=[status_reply(0x0001)])
        status = isp_get_status(dev)
        self.assertEqual(dev.calls, [("raw", b"\x43\x02", 7, 2000)])
        self.assertTrue(status["FULL_IMAGE_RECEIVED"])

    def test_a_reply_to_another_command_is_rejected(self):
        dev = FakeDevice(raw=[struct.pack(">BBH", 0x42, 2, 0)])
        with self.assertRaises(BadAEIResponse):
            isp_get_status(dev)


class TestIspEnter(QuietTestCase):
    def test_enters_isp_mode_and_checks_the_psu_is_ready(self):
        dev = FakeDevice(raw=[enter_reply(), status_reply(0x0002)])
        isp_enter(dev)
        self.assertEqual(dev.calls[0], ("raw", b"\x42\x01", 7, 5000))

    def test_a_psu_which_does_not_go_pending_is_retried_then_reported(self):
        # 5 retries plus the final attempt, two frames each.
        dev = FakeDevice(raw=[enter_reply(), status_reply(0x0000)] * 6)
        with self.assertRaises(BadAEIResponse):
            isp_enter(dev)
        self.assertEqual(len(dev.calls), 12)
        self.assertIn("not ready to receive image", self.stdout.getvalue())

    def test_a_reply_echoing_the_wrong_command(self):
        dev = FakeDevice(raw=[enter_reply(cmd=0x0)] * 6)
        with self.assertRaises(BadAEIResponse):
            isp_enter(dev)

    def test_a_transient_failure_is_retried(self):
        dev = FakeDevice(raw=[ModbusTimeout(), enter_reply(), status_reply(0x0002)])
        isp_enter(dev)
        self.assertEqual(len(dev.calls), 3)


class TestIspExit(QuietTestCase):
    def test_leaves_isp_mode(self):
        dev = FakeDevice(raw=[exit_reply()])
        isp_exit(dev)
        self.assertEqual(dev.calls, [("raw", b"\x42\x00", 7, 5000)])

    def test_a_failure_status_is_reported(self):
        dev = FakeDevice(raw=[exit_reply(status=1)])
        with self.assertRaises(BadAEIResponse):
            isp_exit(dev)

    def test_a_reply_to_another_command_is_rejected(self):
        dev = FakeDevice(raw=[exit_reply(cmd=0x1)])
        with self.assertRaises(BadAEIResponse):
            isp_exit(dev)

    def test_a_timeout_is_expected_as_the_psu_resets(self):
        dev = FakeDevice(raw=[ModbusTimeout()])
        isp_exit(dev)
        self.assertIn("Ignoring timeout of ISP_EXIT", self.stdout.getvalue())


class TestIspContextManager(QuietTestCase):
    def test_exits_isp_mode_after_a_successful_update(self):
        dev = FakeDevice(raw=[enter_reply(), status_reply(0x0002), exit_reply()])
        with isp(dev):
            pass
        self.assertEqual(len(dev.calls), 3)

    def test_exits_isp_mode_after_a_failed_update(self):
        # A PSU left in ISP mode does not power the rack.
        dev = FakeDevice(raw=[enter_reply(), status_reply(0x0002), exit_reply()])
        with self.assertRaises(ValueError):
            with isp(dev):
                raise ValueError("transfer failed")
        self.assertEqual(dev.calls[-1], ("raw", b"\x42\x00", 7, 5000))

    def test_a_failure_to_enter_still_tries_to_exit(self):
        dev = FakeDevice(raw=[enter_reply(cmd=0x0)] * 6 + [exit_reply()])
        with self.assertRaises(BadAEIResponse):
            with isp(dev):
                self.fail("the update must not run")
        self.assertEqual(dev.calls[-1], ("raw", b"\x42\x00", 7, 5000))


class TestIspFlashBlock(QuietTestCase):
    ORV3 = device_params["orv3"]
    HPR = device_params["hpr"]

    def test_orv3_prefixes_the_block_number(self):
        block = bytes(range(64))
        dev = FakeDevice(raw=[block_reply(7)])
        isp_flash_block(dev, 7, block, self.ORV3)
        req = dev.calls[0][1]
        self.assertEqual(req[:4], struct.pack(">BBH", 0x45, 66, 7))
        self.assertEqual(req[4:], block)
        self.assertEqual(dev.calls[0][2], 8)

    def test_hpr_takes_the_block_number_from_the_image(self):
        block = bytes(range(68))
        dev = FakeDevice(raw=[block_reply(0x0001)])
        isp_flash_block(dev, 0, block, self.HPR)
        req = dev.calls[0][1]
        self.assertEqual(req[:2], struct.pack(">BB", 0x45, 68))
        self.assertEqual(req[2:], block)

    def test_a_short_block_is_dropped_rather_than_sent(self):
        dev = FakeDevice()
        isp_flash_block(dev, 0, bytes(10), self.ORV3)
        self.assertEqual(dev.calls, [])
        self.assertIn("Ignoring unexpected block size 10", self.stdout.getvalue())

    def test_a_reply_to_another_command_is_rejected(self):
        dev = FakeDevice(raw=[block_reply(0, func=0x44)])
        with self.assertRaises(BadAEIResponse):
            isp_flash_block(dev, 0, bytes(64), self.ORV3)

    def test_a_reply_of_the_wrong_length_is_rejected(self):
        dev = FakeDevice(raw=[block_reply(0, length=0x4)])
        with self.assertRaises(BadAEIResponse):
            isp_flash_block(dev, 0, bytes(64), self.ORV3)

    def test_a_reply_about_another_block_is_rejected(self):
        dev = FakeDevice(raw=[block_reply(3)])
        with self.assertRaises(BadAEIResponse):
            isp_flash_block(dev, 4, bytes(64), self.ORV3)

    def test_hpr_does_not_check_the_block_number_in_the_reply(self):
        # The PSU picks it out of the image, so the updater has nothing
        # to compare it against.
        dev = FakeDevice(raw=[block_reply(0x1234)])
        isp_flash_block(dev, 0, bytes(68), self.HPR)

    def test_an_already_written_block_is_skipped_with_a_warning(self):
        dev = FakeDevice(raw=[block_reply(2, code=0x1)])
        isp_flash_block(dev, 2, bytes(64), self.ORV3)
        self.assertIn("Skipping block 2", self.stdout.getvalue())

    def test_a_block_outside_the_flash_range_fails_the_update(self):
        dev = FakeDevice(raw=[block_reply(2, code=0x2)])
        with self.assertRaises(BadAEIResponse):
            isp_flash_block(dev, 2, bytes(64), self.ORV3)

    def test_an_unknown_return_code_fails_the_update(self):
        dev = FakeDevice(raw=[block_reply(2, code=0x9)])
        with self.assertRaises(BadAEIResponse):
            isp_flash_block(dev, 2, bytes(64), self.ORV3)


class TestTransferImage(QuietTestCase):
    def test_sends_every_block_in_order(self):
        image = bytes(192)
        dev = FakeDevice(raw=[block_reply(i) for i in range(3)])
        transfer_image(dev, image, device_params["orv3"])
        self.assertEqual(len(dev.calls), 3)
        for i, call in enumerate(dev.calls):
            self.assertEqual(call[1][2:4], i.to_bytes(2, "big"))

    def test_a_partial_block_at_the_end_is_not_sent(self):
        # A workaround for images with spurious trailing bytes.
        dev = FakeDevice(raw=[block_reply(0), block_reply(1)])
        transfer_image(dev, bytes(130), device_params["orv3"])
        self.assertEqual(len(dev.calls), 2)
        self.assertIn("Ignoring partial block", self.stdout.getvalue())

    def test_an_empty_image_sends_nothing(self):
        dev = FakeDevice()
        transfer_image(dev, b"", device_params["orv3"])
        self.assertEqual(dev.calls, [])

    def test_an_image_shorter_than_one_block(self):
        # It rounds down to zero blocks, which the progress line then
        # divides by. Pinned as-is: the update dies on the arithmetic
        # rather than on a check of the image.
        dev = FakeDevice()
        with self.assertRaises(ZeroDivisionError):
            transfer_image(dev, bytes(10), device_params["orv3"])
        self.assertEqual(dev.calls, [])


class TestWaitUpdateComplete(QuietTestCase):
    def test_returns_once_the_psu_reboots(self):
        dev = FakeDevice(raw=[status_reply(0x0001), status_reply(0x0000)])
        wait_update_complete(dev)
        self.assertIn("PSU Rebooted!", self.stdout.getvalue())

    def test_ignores_the_failures_of_a_resetting_psu(self):
        dev = FakeDevice(raw=[ModbusTimeout(), status_reply(0x0000)])
        wait_update_complete(dev)

    def test_reports_each_new_status_bit_once(self):
        dev = FakeDevice(
            raw=[
                status_reply(0x0002),
                status_reply(0x0002),
                status_reply(0x0800),
                status_reply(0x0000),
            ]
        )
        wait_update_complete(dev)
        self.assertEqual(
            self.stdout.getvalue().count("ALERT: FULL_IMAGE_RECV_PENDING"), 1
        )
        self.assertIn("ALERT: PRIMARY_DSP_CHECKSUM_PASSED", self.stdout.getvalue())

    def test_a_psu_which_never_reboots_times_out(self):
        dev = FakeDevice(raw=[status_reply(0x0002)] * 360)
        with self.assertRaises(ValueError) as ctx:
            wait_update_complete(dev)
        self.assertIn("Timed out", str(ctx.exception))

    def test_an_update_failure_is_not_reported_as_such(self):
        # The ValueError naming the failed field is raised inside the
        # try which swallows every exception, so a checksum failure
        # reads as a six minute timeout instead. Pinned to describe what
        # the code does today, not what the message says it does.
        dev = FakeDevice(raw=[status_reply(1 << 8)] * 360)
        with self.assertRaises(ValueError) as ctx:
            wait_update_complete(dev)
        self.assertIn("Timed out", str(ctx.exception))
        self.assertNotIn("PRIMARY_DSP_CHECKSUM_FAILED", str(ctx.exception))


class TestUpdateDevice(QuietTestCase):
    def test_the_whole_sequence(self):
        image = bytes(128)
        dev = FakeDevice(
            raw=[
                enter_reply(),
                status_reply(0x0002),
                block_reply(0),
                block_reply(1),
                status_reply(0x0001),
                exit_reply(),
                status_reply(0x0000),
            ]
        )
        with patch.object(psu_update_aei, "load_file", return_value=image):
            update_device(dev, "fw.bin", device_params["orv3"])
        kinds = [call[1][:1] for call in dev.calls]
        self.assertEqual(
            kinds,
            [b"\x42", b"\x43", b"\x45", b"\x45", b"\x43", b"\x42", b"\x43"],
        )

    def test_a_psu_which_did_not_take_the_whole_image_fails_the_update(self):
        dev = FakeDevice(
            raw=[
                enter_reply(),
                status_reply(0x0002),
                block_reply(0),
                status_reply(0x0002),
                exit_reply(),
            ]
        )
        with patch.object(psu_update_aei, "load_file", return_value=bytes(64)):
            with self.assertRaises(BadAEIResponse):
                update_device(dev, "fw.bin", device_params["orv3"])
        self.assertIn("Did not receive the full image", self.stdout.getvalue())


class TestMain(QuietTestCase):
    def test_reports_the_version_before_and_after(self):
        dev = FakeDevice(read_str=["1.0", "2.0"])
        with patch.object(psu_update_aei, "update_device") as update:
            psu_update_aei.main(dev, "fw.bin", "orv3")
        update.assert_called_once_with(dev, "fw.bin", device_params["orv3"])
        self.assertEqual(dev.calls_of("read_str"), [("read_str", 48, 4, 0)] * 2)
        self.assertIn("Upgrade success", self.stdout.getvalue())

    def test_a_failed_update_exits_non_zero(self):
        dev = FakeDevice(read_str=["1.0"])
        with patch.object(
            psu_update_aei, "update_device", side_effect=BadAEIResponse()
        ):
            with patch("sys.stderr", new=io.StringIO()):
                with self.assertRaises(SystemExit) as ctx:
                    psu_update_aei.main(dev, "fw.bin", "orv3")
        self.assertEqual(ctx.exception.code, 1)
        self.assertIn("Firmware update failed", self.stdout.getvalue())

    def test_an_unknown_device_variant(self):
        with self.assertRaises(KeyError):
            psu_update_aei.main(FakeDevice(), "fw.bin", "nope")


class TestDeviceParams(unittest.TestCase):
    def test_the_variants_the_dispatcher_asks_for_exist(self):
        self.assertEqual(sorted(device_params), ["hpr", "orv3"])

    def test_only_hpr_embeds_the_block_number(self):
        self.assertFalse(device_params["orv3"]["embedded_block_no"])
        self.assertTrue(device_params["hpr"]["embedded_block_no"])
        # The embedded block number takes the four bytes the orv3 prefix
        # would have used.
        self.assertEqual(
            device_params["hpr"]["block_size"],
            device_params["orv3"]["block_size"] + 4,
        )


if __name__ == "__main__":
    unittest.main()
