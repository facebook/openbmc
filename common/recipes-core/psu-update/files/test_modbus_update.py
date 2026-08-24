import importlib.util
import io
import os
import unittest
from contextlib import contextmanager
from unittest.mock import MagicMock, patch

import test_mocks  # noqa: F401  installs the fake pyrmd/minimalmodbus


_HERE = os.path.dirname(os.path.abspath(__file__))
_MODULE_PATH = os.path.join(_HERE, "modbus-update.py")
_module = None


def load_modbus_update():
    """
    Import modbus-update.py.

    Its name is not a python identifier, so it cannot be imported with
    the import statement. It is loaded once and cached, as the tests
    patch attributes on it and expect to be patching the same object.
    """
    global _module
    if _module is None:
        spec = importlib.util.spec_from_file_location("modbus_update", _MODULE_PATH)
        _module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(_module)
    return _module


mu = load_modbus_update()


class FakeDev:
    """A device handle which only has to survive being suppressed"""

    def __init__(self):
        self.suppressed = 0

    @contextmanager
    def suppress_monitoring(self):
        self.suppressed += 1
        yield


class TestDecodeName(unittest.TestCase):
    def test_type_position_and_number(self):
        self.assertEqual(mu.decode_name("BBU_1_2"), ("BBU", 1, 2))
        self.assertEqual(mu.decode_name("PSU_100_3"), ("PSU", 100, 3))

    def test_number_is_optional(self):
        self.assertEqual(mu.decode_name("RPU_200"), ("RPU", 200, None))

    def test_type_is_upper_cased(self):
        self.assertEqual(mu.decode_name("psu_1_2"), ("PSU", 1, 2))

    def test_a_type_may_contain_underscores(self):
        self.assertEqual(mu.decode_name("BBU_PMM_1"), ("BBU_PMM", 1, None))

    def test_rejects_a_name_with_no_position(self):
        for name in ("PSU", "PSU_", "", "1_2"):
            with self.subTest(name=name):
                with self.assertRaises(ValueError):
                    mu.decode_name(name)


class TestIsRackmonPosition(unittest.TestCase):
    def test_shelf_100_and_up_is_a_legacy_orv3_device(self):
        self.assertTrue(mu.is_rackmon_position(100))
        self.assertTrue(mu.is_rackmon_position(203))
        self.assertFalse(mu.is_rackmon_position(99))
        self.assertFalse(mu.is_rackmon_position(1))


class TestGetRackmonDeviceUaddr(unittest.TestCase):
    def test_rpu(self):
        self.assertEqual(mu.get_rackmon_device_uaddr("RPU", 100, None), 0x10C)
        self.assertEqual(mu.get_rackmon_device_uaddr("RPU", 102, None), 0x30C)

    def test_rpu2(self):
        self.assertEqual(mu.get_rackmon_device_uaddr("RPU2", 200, None), 0x10D)
        self.assertEqual(mu.get_rackmon_device_uaddr("RPU2", 202, None), 0x30D)

    def test_an_rpu_has_no_device_number(self):
        for dev_type in ("RPU", "RPU2"):
            with self.subTest(dev_type=dev_type):
                with self.assertRaises(ValueError):
                    mu.get_rackmon_device_uaddr(dev_type, 100, 1)

    def test_orv3_psu(self):
        # rack 0, PSU 1: dtype=3, r2=1 -> 0x1E0
        self.assertEqual(mu.get_rackmon_device_uaddr("ORV3_PSU", 100, 1), 0x1E0)
        self.assertEqual(mu.get_rackmon_device_uaddr("ORV3_PSU", 100, 2), 0x1E1)

    def test_orv3_bbu(self):
        # rack 1, BBU 2: dtype=1, r2=0 -> 0x249
        self.assertEqual(mu.get_rackmon_device_uaddr("ORV3_BBU", 101, 2), 0x249)

    def test_the_shelf_ends_up_in_the_upper_byte(self):
        for position in range(100, 104):
            uaddr = mu.get_rackmon_device_uaddr("ORV3_PSU", position, 1)
            with self.subTest(position=position):
                self.assertEqual(uaddr >> 8, position - 99)

    def test_addresses_of_one_shelf_are_distinct(self):
        seen = {
            mu.get_rackmon_device_uaddr(dev_type, 100, num)
            for dev_type in ("ORV3_PSU", "ORV3_BBU")
            for num in range(1, 7)
        }
        self.assertEqual(len(seen), 12)

    def test_unknown_device_type(self):
        with self.assertRaises(ValueError):
            mu.get_rackmon_device_uaddr("PSU", 100, 1)


class TestGetUpdater(unittest.TestCase):
    def setUp(self):
        self.dev = FakeDev()
        for name in (
            "psu_update_aei",
            "psu_update_delta_orv3",
            "orv3_device_update_mailbox",
            "rpu_update_delta_plc",
            "rpu_update_delta_hex",
            "rpu_update_coolermaster",
        ):
            patcher = patch.object(mu, name, MagicMock())
            setattr(self, name, patcher.start())
            self.addCleanup(patcher.stop)

    def test_orv3_psu_artesyn_is_an_aei_update(self):
        mu.get_updater("ORV3_PSU", "artesyn", None)(self.dev, "fw.bin")
        self.psu_update_aei.main.assert_called_once_with(self.dev, "fw.bin", "orv3")

    def test_hpr_psu_artesyn_uses_the_hpr_variant(self):
        mu.get_updater("PSU", "artesyn", None)(self.dev, "fw.bin")
        self.psu_update_aei.main.assert_called_once_with(self.dev, "fw.bin", "hpr")

    def test_delta_psu_is_a_hex_file_update(self):
        mu.get_updater("PSU", "delta", None)(self.dev, "fw.hex")
        self.psu_update_delta_orv3.main.assert_called_once_with(self.dev, "fw.hex")

    def test_bbu_vendors_pick_the_mailbox_variant(self):
        mu.get_updater("ORV3_BBU", "panasonic", None)(self.dev, "fw.bin")
        self.orv3_device_update_mailbox.main.assert_called_once_with(
            self.dev, "fw.bin", "panasonic"
        )

    def test_a_pmm_is_updated_as_its_own_vendor(self):
        for dev_type in ("PSU_PMM", "BBU_PMM", "CBU_PMM"):
            self.orv3_device_update_mailbox.reset_mock()
            with self.subTest(dev_type=dev_type):
                mu.get_updater(dev_type, "delta", None)(self.dev, "fw.bin")
                self.orv3_device_update_mailbox.main.assert_called_once_with(
                    self.dev, "fw.bin", "hpr_pmm_delta"
                )

    def test_cbu_has_its_own_variant(self):
        mu.get_updater("CBU", "delta", None)(self.dev, "fw.bin")
        self.orv3_device_update_mailbox.main.assert_called_once_with(
            self.dev, "fw.bin", "delta_cbu"
        )

    def test_rpu_defaults_to_the_plc(self):
        mu.get_updater("RPU", "delta", None)(self.dev, "fw.txt")
        self.rpu_update_delta_plc.main.assert_called_once_with(self.dev, "fw.txt", True)

    def test_a_non_delta_rpu_plc_does_not_use_oem_blocks(self):
        mu.get_updater("RPU", "quanta", "PLC")(self.dev, "fw.txt")
        self.rpu_update_delta_plc.main.assert_called_once_with(
            self.dev, "fw.txt", False
        )

    def test_rpu_hex_component(self):
        mu.get_updater("RPU", "delta", "HEX")(self.dev, "fw.bin")
        self.rpu_update_delta_hex.main.assert_called_once_with(self.dev, "fw.bin")

    def test_rpu2_is_always_a_coolermaster(self):
        mu.get_updater("RPU2", "coolermaster", None)(self.dev, "fw.bin")
        self.rpu_update_coolermaster.main.assert_called_once_with(
            self.dev, "fw.bin", None
        )

    def test_an_rpu2_component_is_passed_through(self):
        mu.get_updater("RPU2", "coolermaster", "FAN_RACK_1_ETH")(self.dev, "fw.tar.gz")
        self.rpu_update_coolermaster.main.assert_called_once_with(
            self.dev, "fw.tar.gz", "FAN_RACK_1_ETH"
        )

    def test_an_unknown_rpu2_component_exits_before_the_device_is_touched(self):
        with patch("sys.stdout", new=io.StringIO()) as out:
            with self.assertRaises(SystemExit):
                mu.get_updater("RPU2", "coolermaster", "FAN_RACK_9_ETH")
        self.assertIn("Unsupported RPU2 component: FAN_RACK_9_ETH", out.getvalue())
        self.rpu_update_coolermaster.main.assert_not_called()

    def test_unsupported_component_exits(self):
        with patch("sys.stdout", new=io.StringIO()) as out:
            with self.assertRaises(SystemExit):
                mu.get_updater("RPU", "delta", "FPGA")
        self.assertIn("Unsupported RPU component: FPGA", out.getvalue())

    def test_unsupported_device_type_exits(self):
        with patch("sys.stdout", new=io.StringIO()) as out:
            with self.assertRaises(SystemExit):
                mu.get_updater("TOASTER", "delta", None)
        self.assertIn("Unsupported device type: TOASTER", out.getvalue())

    def test_unsupported_vendor_exits_naming_the_device(self):
        with patch("sys.stdout", new=io.StringIO()) as out:
            with self.assertRaises(SystemExit):
                mu.get_updater("BBU_PMM", "acme", None)
        self.assertIn("Unsupported PMM Vendor: acme", out.getvalue())

    def test_an_undetected_vendor_is_reported_rather_than_guessed(self):
        # get_manufacturer() returns None when it does not recognise the
        # device; that must not fall through to some default updater.
        with patch("sys.stdout", new=io.StringIO()):
            with self.assertRaises(SystemExit):
                mu.get_updater("PSU", None, None)

    def test_every_updater_can_be_named_in_the_dry_run_output(self):
        for dev_type, (_, vendors) in mu.UPDATERS.items():
            for vendor in vendors:
                update = mu.get_updater(dev_type, vendor, None)
                with self.subTest(dev_type=dev_type, vendor=vendor):
                    self.assertTrue(getattr(update, "description", update.__name__))


class TestGetRackmonDeviceConfig(unittest.TestCase):
    def test_finds_the_device_rackmon_knows_about(self):
        with patch.object(mu.rmd, "list", return_value=[{"uniqueDevAddress": 0x1E0}]):
            self.assertEqual(
                mu.get_rackmon_device_config(0x1E0), {"uniqueDevAddress": 0x1E0}
            )

    def test_unknown_address(self):
        with patch.object(mu.rmd, "list", return_value=[{"uniqueDevAddress": 0x1E1}]):
            with self.assertRaises(ValueError):
                mu.get_rackmon_device_config(0x1E0)


class TestGetDevice(unittest.TestCase):
    def test_a_legacy_shelf_goes_over_rackmon(self):
        with patch.object(mu, "get_rackmon_device") as get_rackmon:
            dev_type, dev = mu.get_device("PSU_100_1", None)
        self.assertEqual(dev_type, "ORV3_PSU")
        get_rackmon.assert_called_once_with(
            "PSU_100_1", "ORV3_PSU", 100, 1, None, False
        )
        self.assertIs(dev, get_rackmon.return_value)

    def test_bbu_on_a_legacy_shelf(self):
        with patch.object(mu, "get_rackmon_device") as get_rackmon:
            dev_type, _ = mu.get_device("BBU_101_2", None)
        self.assertEqual(dev_type, "ORV3_BBU")
        get_rackmon.assert_called_once_with(
            "BBU_101_2", "ORV3_BBU", 101, 2, None, False
        )

    def test_an_explicit_unique_address_forces_rackmon(self):
        # Even at a shelf which would otherwise be phosphor-modbus.
        with patch.object(mu, "get_rackmon_device") as get_rackmon:
            dev_type, _ = mu.get_device("PSU_1_1", 0x1E0)
        self.assertEqual(dev_type, "ORV3_PSU")
        get_rackmon.assert_called_once_with("PSU_1_1", "ORV3_PSU", 1, 1, 0x1E0, False)

    def test_rpu_shelf_selects_the_rpu_generation(self):
        with patch.object(mu, "get_rackmon_device") as get_rackmon:
            self.assertEqual(mu.get_device("RPU_100", None)[0], "RPU")
            self.assertEqual(mu.get_device("RPU_200", None)[0], "RPU2")
        self.assertEqual(get_rackmon.call_count, 2)

    def test_a_type_rackmon_cannot_reach(self):
        with self.assertRaises(ValueError):
            mu.get_device("CBU_100_1", None)

    def test_a_modern_shelf_goes_over_phosphor_modbus(self):
        with patch.object(mu, "get_phosphor_modbus_device") as get_pmodbus:
            dev_type, dev = mu.get_device("BBU_PMM_1", None)
        self.assertEqual(dev_type, "BBU_PMM")
        get_pmodbus.assert_called_once_with("BBU_PMM_1")
        self.assertIs(dev, get_pmodbus.return_value)

    def test_the_type_is_not_rewritten_on_the_phosphor_modbus_path(self):
        with patch.object(mu, "get_phosphor_modbus_device"):
            self.assertEqual(mu.get_device("PSU_1_1", None)[0], "PSU")


class TestGetRackmonDevice(unittest.TestCase):
    def test_validates_the_address_before_handing_back_a_handle(self):
        with patch.object(mu, "get_rackmon_device_config") as get_config:
            with patch.object(mu, "ModbusRackmon") as modbus:
                dev = mu.get_rackmon_device("PSU_100_1", "ORV3_PSU", 100, 1, None)
        get_config.assert_called_once_with(0x1E0)
        modbus.assert_called_once_with(0x1E0)
        self.assertIs(dev, modbus.return_value)

    def test_an_explicit_address_is_used_as_is(self):
        with patch.object(mu, "get_rackmon_device_config"):
            with patch.object(mu, "ModbusRackmon") as modbus:
                mu.get_rackmon_device("PSU_100_1", "ORV3_PSU", 100, 1, 0x242)
        modbus.assert_called_once_with(0x242)

    def test_an_unknown_device_is_not_driven(self):
        with patch.object(mu, "get_rackmon_device_config", side_effect=ValueError):
            with patch.object(mu, "ModbusRackmon") as modbus:
                with self.assertRaises(ValueError):
                    mu.get_rackmon_device("PSU_100_1", "ORV3_PSU", 100, 1, None)
        modbus.assert_not_called()


class TestGetRackmonDeviceForcedDirect(unittest.TestCase):
    """--force-direct: a rackmon device driven over minimalmodbus instead"""

    def config(self):
        return {
            "uniqueDevAddress": 0x1E0,
            "devAddress": 0xE0,
            "baudrate": 19200,
            "parity": "EVEN",
        }

    def test_the_line_settings_come_from_rackmons_own_device_config(self):
        with patch.object(mu, "get_rackmon_device_config", return_value=self.config()):
            with patch.object(
                mu.rmd, "get_interface", return_value="/dev/ttyRS485-1"
            ) as get_interface:
                with patch.object(mu, "ModbusDirect") as modbus:
                    with patch.object(mu, "RackmonMonitor") as monitor:
                        dev = mu.get_rackmon_device(
                            "PSU_100_1", "ORV3_PSU", 100, 1, None, True
                        )
        get_interface.assert_called_once_with(0x1E0)
        modbus.assert_called_once_with(
            0xE0, 19200, "EVEN", "/dev/ttyRS485-1", monitor.return_value
        )
        self.assertIs(dev, modbus.return_value)

    def test_rackmon_is_still_the_monitor_to_suppress(self):
        # The device is ours to drive, but rackmond is still the daemon
        # polling it, so that is what has to stand off.
        with patch.object(mu, "get_rackmon_device_config", return_value=self.config()):
            with patch.object(mu.rmd, "get_interface", return_value="/dev/ttyRS485-1"):
                with patch.object(mu, "ModbusDirect") as modbus:
                    mu.get_rackmon_device("PSU_100_1", "ORV3_PSU", 100, 1, None, True)
        monitor = modbus.call_args.args[4]
        self.assertIsInstance(monitor, mu.RackmonMonitor)

    def test_without_minimalmodbus_there_is_no_direct_backend(self):
        with patch.object(mu, "get_rackmon_device_config", return_value=self.config()):
            with patch.object(mu, "ModbusDirect", None):
                with self.assertRaises(ValueError):
                    mu.get_rackmon_device("PSU_100_1", "ORV3_PSU", 100, 1, None, True)


class TestGetPhosphorModbusDevice(unittest.TestCase):
    def config(self):
        cfg = MagicMock()
        cfg.baudrate = 19200
        cfg.address = 0x32
        cfg.device_path = "/dev/ttyRS485-1"
        cfg.parity_char = "E"
        return cfg

    def test_line_settings_come_from_the_device_config(self):
        cfg = self.config()
        with patch.object(mu, "get_pmodbus_config", return_value=cfg):
            with patch.object(mu, "ModbusDirect") as modbus:
                dev = mu.get_phosphor_modbus_device("BBU_1_1")
        modbus.assert_called_once_with(0x32, 19200, "E", "/dev/ttyRS485-1")
        self.assertIs(dev, modbus.return_value)

    def test_without_minimalmodbus_there_is_no_direct_backend(self):
        with patch.object(mu, "get_pmodbus_config", return_value=self.config()):
            with patch.object(mu, "ModbusDirect", None):
                with self.assertRaises(ValueError):
                    mu.get_phosphor_modbus_device("BBU_1_1")

    def test_looks_the_device_up_by_inventory_name(self):
        cfg = self.config()
        with patch.object(
            mu.phosphor_modbus, "get_all_device_configs", return_value={"BBU_1_1": cfg}
        ):
            self.assertIs(mu.get_pmodbus_config("BBU_1_1"), cfg)
            with self.assertRaises(KeyError):
                mu.get_pmodbus_config("BBU_1_2")


class TestMain(unittest.TestCase):
    def run_main(self, argv, dev, vendor="delta"):
        with patch("sys.argv", ["modbus-update.py"] + argv):
            with patch.object(mu, "get_device", return_value=("PSU", dev)):
                with patch.object(
                    mu.manufacturers, "get_manufacturer", return_value=vendor
                ) as get_manufacturer:
                    with patch.object(mu, "get_updater") as get_updater:
                        get_updater.return_value.__name__ = "fake_updater"
                        with patch("sys.stdout", new=io.StringIO()) as out:
                            mu.main()
        return get_manufacturer, get_updater, out.getvalue()

    def test_the_update_runs_with_monitoring_suppressed(self):
        dev = FakeDev()
        _, get_updater, _ = self.run_main(["-n", "PSU_1_1", "fw.bin"], dev)
        self.assertEqual(dev.suppressed, 1)
        get_updater.return_value.assert_called_once_with(dev, "fw.bin")

    def test_the_vendor_is_detected_from_the_device(self):
        dev = FakeDev()
        get_manufacturer, get_updater, _ = self.run_main(
            ["-n", "PSU_1_1", "fw.bin"], dev, vendor="artesyn"
        )
        get_manufacturer.assert_called_once_with("PSU", dev)
        get_updater.assert_called_once_with("PSU", "artesyn", None)

    def test_the_component_is_passed_through(self):
        _, get_updater, _ = self.run_main(
            ["-n", "RPU_1", "-c", "HEX", "fw.bin"], FakeDev()
        )
        get_updater.assert_called_once_with("PSU", "delta", "HEX")

    def test_a_dry_run_stops_before_the_update(self):
        dev = FakeDev()
        _, get_updater, out = self.run_main(
            ["-n", "PSU_1_1", "--dry-run", "fw.bin"], dev
        )
        get_updater.return_value.assert_not_called()
        self.assertIn("Dry run", out)
        self.assertIn("Device Type: PSU", out)
        self.assertIn("Vendor: delta", out)
        self.assertIn("File: fw.bin", out)
        # Monitoring was still suppressed to read the vendor out, and
        # resumed on the way out.
        self.assertEqual(dev.suppressed, 1)

    def resolve_device(self, argv):
        """The get_device() mock main() resolved its device through"""
        with patch("sys.argv", ["modbus-update.py"] + argv):
            with patch.object(mu, "get_device", return_value=("PSU", FakeDev())) as gd:
                with patch.object(mu.manufacturers, "get_manufacturer"):
                    with patch.object(mu, "get_updater"):
                        with patch("sys.stdout", new=io.StringIO()):
                            mu.main()
        return gd

    def test_the_address_option_forces_a_rackmon_lookup(self):
        gd = self.resolve_device(["-n", "PSU_1_1", "-a", "0x1e0", "f"])
        gd.assert_called_once_with("PSU_1_1", 0x1E0, False)

    def test_the_backend_is_rackmons_own_unless_told_otherwise(self):
        gd = self.resolve_device(["-n", "PSU_100_1", "f"])
        gd.assert_called_once_with("PSU_100_1", None, False)

    def test_force_direct_is_passed_through(self):
        gd = self.resolve_device(["-n", "PSU_100_1", "--force-direct", "f"])
        gd.assert_called_once_with("PSU_100_1", None, True)


if __name__ == "__main__":
    unittest.main()
