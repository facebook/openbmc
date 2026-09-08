import io
import os
import tempfile
import unittest
from unittest.mock import patch

import orv3_device_update_mailbox as mailbox
from modbus_impl_pyrmd import ModbusCRCError, ModbusTimeout
from orv3_device_update_mailbox import (
    boot_mode,
    enter_boot_mode,
    ENTERED_BOOT_MODE,
    exit_boot_mode,
    FIRMWARE_PACKET_CORRECT,
    FIRMWARE_UPGRADE_SUCCESS,
    get_firmware_status,
    load_file,
    NORMAL_OPERATION_MODE,
    transfer_image,
    unlock_firmware,
    update_device,
    vendor_params,
    verify_firmware,
    verify_firmware_status,
    wait_write_block,
    workaround_force_exit_boot_mode,
    write_block,
)

# Imported before the modules under test: it puts the fake pyrmd into
# sys.modules, which the updater needs to be importable at all.
from test_mocks import FakeDevice


class QuietTestCase(unittest.TestCase):
    def setUp(self):
        patcher = patch("sys.stdout", new=io.StringIO())
        self.stdout = patcher.start()
        self.addCleanup(patcher.stop)
        # Both the updater and the retry decorator sleep on the way
        # through; one patch covers both.
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

    def test_an_empty_file(self):
        self.assertEqual(self.load(b""), [])


class TestMailboxRegisters(QuietTestCase):
    def test_unlock_writes_the_engineering_mode_key(self):
        dev = FakeDevice()
        unlock_firmware(dev)
        self.assertEqual(dev.calls, [("write", 0x300, 0x55AA, 1000)])

    def test_get_firmware_status_reads_the_status_register(self):
        dev = FakeDevice(read=[[ENTERED_BOOT_MODE]])
        self.assertEqual(get_firmware_status(dev), ENTERED_BOOT_MODE)
        self.assertEqual(dev.calls, [("read", 0x302, 1, 1000)])

    def test_verify_firmware_status_accepts_the_expected_state(self):
        dev = FakeDevice(read=[[FIRMWARE_UPGRADE_SUCCESS]])
        verify_firmware_status(dev, FIRMWARE_UPGRADE_SUCCESS)

    def test_verify_firmware_status_reports_the_state_it_found(self):
        dev = FakeDevice(read=[[0x55]] * 6)
        with self.assertRaises(ValueError) as ctx:
            verify_firmware_status(dev, FIRMWARE_UPGRADE_SUCCESS)
        self.assertIn("0x55", str(ctx.exception))
        self.assertIn("0xaa", str(ctx.exception))

    def test_verify_firmware_status_retries_a_device_which_is_still_settling(self):
        dev = FakeDevice(read=[[WAIT] for WAIT in (0x18, 0x18)] + [[0xAA]])
        verify_firmware_status(dev, FIRMWARE_UPGRADE_SUCCESS)
        self.assertEqual(len(dev.calls), 3)

    def test_verify_firmware_asks_the_device_to_check_the_image(self):
        dev = FakeDevice()
        verify_firmware(dev)
        self.assertEqual(dev.calls, [("write", 0x303, 0x55AA, 10000)])


class TestBootMode(QuietTestCase):
    def test_enter_writes_the_boot_key_and_confirms_the_state(self):
        dev = FakeDevice(read=[[ENTERED_BOOT_MODE]])
        enter_boot_mode(dev, 0xA5A5)
        self.assertEqual(dev.calls[0], ("write", 0x301, 0xAA55, 5000))

    def test_the_boot_mode_argument_is_not_used(self):
        # vendor_params carries a per-vendor boot_mode (0xA5A5 for
        # Delta), but the register is always written with 0xAA55.
        # Pinned so that wiring the parameter up is a deliberate change.
        dev = FakeDevice(read=[[ENTERED_BOOT_MODE]])
        enter_boot_mode(dev, 0x1234)
        self.assertEqual(dev.calls[0][2], 0xAA55)

    def test_enter_retries_a_device_which_did_not_take_the_key(self):
        dev = FakeDevice(read=[[NORMAL_OPERATION_MODE]] * 6 + [[ENTERED_BOOT_MODE]])
        enter_boot_mode(dev, 0xAA55)
        self.assertEqual(len(dev.calls_of("write")), 2)

    def test_exit_writes_the_exit_key(self):
        dev = FakeDevice()
        exit_boot_mode(dev)
        self.assertEqual(dev.calls, [("write", 0x304, 0x55AA, 10000)])

    def test_a_device_which_resets_before_acknowledging_the_exit(self):
        # The write times out because the device already left boot mode.
        dev = FakeDevice(read=[[NORMAL_OPERATION_MODE]])
        with patch.object(dev, "write", side_effect=ModbusTimeout()):
            exit_boot_mode(dev)
        self.assertIn("Exit boot mode timed out", self.stdout.getvalue())

    def test_a_device_which_is_still_in_boot_mode_after_the_exit(self):
        dev = FakeDevice(read=[[ENTERED_BOOT_MODE]] * 36)
        with patch.object(dev, "write", side_effect=ModbusTimeout()):
            with self.assertRaises(ValueError):
                exit_boot_mode(dev)

    def test_the_context_manager_leaves_boot_mode_on_the_way_out(self):
        dev = FakeDevice(read=[[ENTERED_BOOT_MODE], [NORMAL_OPERATION_MODE]])
        with boot_mode(dev, 0xAA55):
            pass
        self.assertEqual([call[1] for call in dev.calls_of("write")], [0x301, 0x304])

    def test_a_failed_update_still_leaves_boot_mode(self):
        # A device left in boot mode is not doing its job.
        dev = FakeDevice(read=[[ENTERED_BOOT_MODE], [NORMAL_OPERATION_MODE]])
        with self.assertRaises(ValueError):
            with boot_mode(dev, 0xAA55):
                raise ValueError("transfer failed")
        self.assertEqual(dev.calls_of("write")[-1][1], 0x304)


class TestWriteBlock(QuietTestCase):
    def test_writes_a_full_block_to_the_data_register(self):
        dev = FakeDevice()
        write_block(dev, [1, 2, 3, 4], 4, [])
        self.assertEqual(dev.calls, [("write", 0x310, [1, 2, 3, 4], 1000)])

    def test_a_short_block_is_dropped(self):
        # A workaround for .bin files with spurious trailing bytes.
        dev = FakeDevice()
        write_block(dev, [1, 2], 4, [])
        self.assertEqual(dev.calls, [])

    def test_an_oversized_block_is_a_programming_error(self):
        dev = FakeDevice()
        with self.assertRaises(AssertionError):
            write_block(dev, [1, 2, 3, 4, 5], 4, [])

    def test_a_crc_error_fails_the_write_by_default(self):
        dev = FakeDevice()
        with patch.object(dev, "write", side_effect=ModbusCRCError()):
            with self.assertRaises(ModbusCRCError):
                write_block(dev, [1, 2], 2, [])

    def test_early_bootloaders_may_have_their_bad_crc_suppressed(self):
        workarounds = ["WRITE_BLOCK_CRC_EXPECTED"]
        dev = FakeDevice()
        with patch.object(dev, "write", side_effect=ModbusCRCError()):
            write_block(dev, [1, 2], 2, workarounds)
            write_block(dev, [1, 2], 2, workarounds)
        self.assertEqual(self.stdout.getvalue().count("CRCError suppressed"), 1)
        self.assertIn("PRINTED", workarounds)

    def test_wait_write_block_waits_for_the_packet_to_be_accepted(self):
        dev = FakeDevice(read=[[0], [0], [FIRMWARE_PACKET_CORRECT]])
        wait_write_block(dev)
        self.assertEqual(len(dev.calls), 3)


class TestTransferImage(QuietTestCase):
    def test_sends_the_image_one_block_at_a_time(self):
        dev = FakeDevice()
        transfer_image(dev, list(range(8)), 4, False, [])
        self.assertEqual(
            [call[2] for call in dev.calls_of("write")],
            [[0, 1, 2, 3], [4, 5, 6, 7]],
        )

    def test_a_partial_block_at_the_end_is_counted_but_not_sent(self):
        dev = FakeDevice()
        transfer_image(dev, list(range(6)), 4, False, [])
        self.assertEqual([call[2] for call in dev.calls_of("write")], [[0, 1, 2, 3]])

    def test_a_device_which_acknowledges_each_block_is_waited_for(self):
        dev = FakeDevice(read=[[FIRMWARE_PACKET_CORRECT]] * 2)
        transfer_image(dev, list(range(8)), 4, True, [])
        self.assertEqual(len(dev.calls_of("read")), 2)

    def test_a_device_which_does_not_is_given_a_fixed_pause(self):
        dev = FakeDevice()
        transfer_image(dev, list(range(8)), 4, False, [])
        self.assertEqual(dev.calls_of("read"), [])
        self.sleep.assert_called_with(0.1)


class TestForceExitBootModeWorkaround(QuietTestCase):
    def test_a_device_already_in_normal_mode_is_left_alone(self):
        dev = FakeDevice(read=[[NORMAL_OPERATION_MODE]])
        workaround_force_exit_boot_mode(dev, [])
        self.assertEqual(dev.calls_of("write"), [])

    def test_a_device_stuck_in_boot_mode_is_walked_through_an_abort(self):
        dev = FakeDevice(
            read=[
                [ENTERED_BOOT_MODE],  # current mode
                [NORMAL_OPERATION_MODE],  # after the forced exit
            ]
        )
        workaround_force_exit_boot_mode(dev, ["FORCE_EXIT_BOOT_MODE"])
        self.assertEqual(
            [call[1] for call in dev.calls_of("write")], [0x303, 0x304, 0x300]
        )

    def test_some_devices_need_the_verify_register_cleared_by_hand(self):
        dev = FakeDevice(read=[[ENTERED_BOOT_MODE], [NORMAL_OPERATION_MODE]])
        workaround_force_exit_boot_mode(
            dev, ["FORCE_EXIT_BOOT_MODE", "FORCE_CLEAR_VERIFY"]
        )
        writes = [(call[1], call[2]) for call in dev.calls_of("write")]
        self.assertIn((0x303, 0), writes)

    def test_a_device_which_will_not_come_back_does_not_stop_the_upgrade(self):
        dev = FakeDevice(read=[[ENTERED_BOOT_MODE], [ENTERED_BOOT_MODE]])
        workaround_force_exit_boot_mode(dev, ["FORCE_EXIT_BOOT_MODE"])
        self.assertIn("Continuing upgrade hoping for the best", self.stdout.getvalue())


class TestUpdateDevice(QuietTestCase):
    def test_the_whole_sequence(self):
        dev = FakeDevice(
            read=[
                [ENTERED_BOOT_MODE],  # enter_boot_mode
                [FIRMWARE_UPGRADE_SUCCESS],  # after the verify
                [NORMAL_OPERATION_MODE],  # after leaving boot mode
            ]
        )
        with patch.object(mailbox, "load_file", return_value=list(range(32))):
            update_device(dev, "fw.bin", vendor_params["delta_cbu"])
        self.assertEqual(
            [call[1] for call in dev.calls_of("write")],
            [0x300, 0x301, 0x310, 0x303, 0x304],
        )

    def test_the_force_exit_workaround_runs_before_the_upgrade(self):
        dev = FakeDevice(
            read=[
                [NORMAL_OPERATION_MODE],  # workaround: nothing to do
                [ENTERED_BOOT_MODE],
                [FIRMWARE_PACKET_CORRECT],
                [FIRMWARE_UPGRADE_SUCCESS],
                [NORMAL_OPERATION_MODE],
            ]
        )
        with patch.object(mailbox, "load_file", return_value=list(range(34))):
            update_device(dev, "fw.bin", vendor_params["hpr_pmm_delta"])
        self.assertEqual(len(dev.calls_of("write")), 5)

    def test_an_image_the_device_rejects_fails_the_update(self):
        dev = FakeDevice(
            read=[[ENTERED_BOOT_MODE]]
            + [[0x55]] * 6  # the verify never reports success
            + [[NORMAL_OPERATION_MODE]]
        )
        with patch.object(mailbox, "load_file", return_value=list(range(32))):
            with self.assertRaises(ValueError):
                update_device(dev, "fw.bin", vendor_params["delta_cbu"])
        # Still left boot mode on the way out.
        self.assertEqual(dev.calls_of("write")[-1][1], 0x304)


class TestPrintRevision(QuietTestCase):
    def test_reads_every_version_register_of_the_vendor(self):
        dev = FakeDevice(read_str=["1.0"])
        mailbox.print_revision(dev, vendor_params["delta"])
        self.assertEqual(dev.calls_of("read_str"), [("read_str", 56, 4, 0)])
        self.assertIn("Version:  1.0", self.stdout.getvalue())

    def test_a_device_with_several_version_registers(self):
        params = vendor_params["delta_miniups"]
        regs = params["version_regs"]
        dev = FakeDevice(read_str=["v%d" % i for i in range(len(regs))])
        mailbox.print_revision(dev, params)
        self.assertEqual(len(dev.calls_of("read_str")), len(params["version_regs"]))


class TestMain(QuietTestCase):
    def test_reports_the_version_before_and_after(self):
        dev = FakeDevice(read_str=["1.0", "2.0"])
        with patch.object(mailbox, "update_device") as update:
            mailbox.main(dev, "fw.bin", "delta")
        update.assert_called_once_with(dev, "fw.bin", vendor_params["delta"])
        self.assertIn("Upgrade success", self.stdout.getvalue())

    def test_the_block_size_can_be_overridden(self):
        original = vendor_params["delta"]["block_size"]
        self.addCleanup(
            lambda: vendor_params["delta"].__setitem__("block_size", original)
        )
        dev = FakeDevice(read_str=["1.0", "2.0"])
        with patch.object(mailbox, "update_device"):
            mailbox.main(dev, "fw.bin", "delta", block_size=96)
        self.assertEqual(vendor_params["delta"]["block_size"], 96)

    def test_a_failed_update_exits_non_zero(self):
        dev = FakeDevice(read_str=["1.0"])
        with patch.object(mailbox, "update_device", side_effect=ValueError("boom")):
            with patch("sys.stderr", new=io.StringIO()):
                with self.assertRaises(SystemExit) as ctx:
                    mailbox.main(dev, "fw.bin", "delta")
        self.assertEqual(ctx.exception.code, 1)

    def test_an_unknown_vendor(self):
        with self.assertRaises(KeyError):
            mailbox.main(FakeDevice(), "fw.bin", "acme")


class TestVendorParams(unittest.TestCase):
    def test_every_vendor_is_fully_specified(self):
        for vendor, params in vendor_params.items():
            with self.subTest(vendor=vendor):
                self.assertGreater(params["block_size"], 0)
                self.assertEqual(params["block_size"] % 2, 0)
                self.assertIn(params["boot_mode"], (0xA5A5, 0xAA55))
                self.assertIsInstance(params["block_wait"], bool)
                self.assertTrue(params["version_regs"])

    def test_the_variants_the_dispatcher_asks_for_exist(self):
        for variant in (
            "delta",
            "delta_cbu",
            "panasonic",
            "hpr_panasonic",
            "hpr_pmm_delta",
            "hpr_pmm_aei",
            "hpr_pmm_panasonic",
        ):
            with self.subTest(variant=variant):
                self.assertIn(variant, vendor_params)

    def test_only_known_workarounds_are_asked_for(self):
        known = {
            "FORCE_EXIT_BOOT_MODE",
            "FORCE_CLEAR_VERIFY",
            "WRITE_BLOCK_CRC_EXPECTED",
        }
        for vendor, params in vendor_params.items():
            with self.subTest(vendor=vendor):
                self.assertLessEqual(set(params.get("hw_workarounds", [])), known)


if __name__ == "__main__":
    unittest.main()
