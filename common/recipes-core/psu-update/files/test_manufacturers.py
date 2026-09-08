import contextlib
import io
import json
import os
import unittest
from unittest.mock import patch

import manufacturers
from manufacturers import get_determinator, get_manufacturer, normalize
from test_mocks import FakeDevice

# The determinator is owned by modbus-device-util. Prefer the copy in the
# tree so the tests cover what a change is about to ship, and fall back to
# the installed one when running out of the package on a target.
_TREE_CONFIG = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..",
    "..",
    "modbus-device-util",
    "files",
    "default-config.json",
)


def find_config():
    for path in (_TREE_CONFIG, manufacturers.DEFAULT_CONFIG_PATH):
        if os.path.exists(path):
            return path
    return None


CONFIG = find_config()
needs_config = unittest.skipIf(
    CONFIG is None, "modbus-device-util default-config.json is not available"
)


def tmpdir(test):
    import shutil
    import tempfile

    d = tempfile.mkdtemp()
    test.addCleanup(shutil.rmtree, d)
    return d


class TestNormalize(unittest.TestCase):
    def test_strips_the_padding_a_register_is_filled_with(self):
        self.assertEqual(normalize("Delta\x00\x00\x00"), "Delta")
        self.assertEqual(normalize("Delta   "), "Delta")
        self.assertEqual(normalize("Delta\r\n"), "Delta")
        self.assertEqual(normalize("Delta\t"), "Delta")

    def test_keeps_leading_padding(self):
        # The regex matchers count characters from the start of the
        # register, so stripping the head would shift them.
        self.assertEqual(normalize("  Delta"), "  Delta")
        self.assertEqual(normalize("\x00\x00Delta\x00"), "\x00\x00Delta")

    def test_leaves_an_unpadded_value_alone(self):
        self.assertEqual(normalize("Delta"), "Delta")
        self.assertEqual(normalize(""), "")


class TestConfigPath(unittest.TestCase):
    def setUp(self):
        self.env = os.environ.pop(manufacturers.CONFIG_PATH_ENV, None)

    def tearDown(self):
        if self.env is None:
            os.environ.pop(manufacturers.CONFIG_PATH_ENV, None)
        else:
            os.environ[manufacturers.CONFIG_PATH_ENV] = self.env

    def test_the_environment_wins(self):
        os.environ[manufacturers.CONFIG_PATH_ENV] = "/tmp/somewhere.json"
        self.assertEqual(manufacturers.get_config_path(), "/tmp/somewhere.json")

    def test_the_shipped_config_is_used_without_an_override(self):
        # The override lives on the persistent partition, which is not
        # mounted where these tests run.
        if os.path.exists(manufacturers.OVERRIDE_CONFIG_PATH):
            self.skipTest("an override config is installed on this machine")
        self.assertEqual(
            manufacturers.get_config_path(), manufacturers.DEFAULT_CONFIG_PATH
        )

    def test_the_override_is_preferred_over_the_shipped_config(self):
        # Same order the modbus-device-util shell scripts resolve in.
        self.assertEqual(
            manufacturers.OVERRIDE_CONFIG_PATH,
            "/run/mnt-persist/var-data/lib/modbus-device-util/override-config.json",
        )
        self.assertEqual(
            manufacturers.DEFAULT_CONFIG_PATH,
            "/var/lib/modbus-device-util/default-config.json",
        )

    @needs_config
    def test_the_config_is_read_from_the_path_it_is_given(self):
        with open(CONFIG) as f:
            config = json.load(f)
        del config["rpu"]
        path = os.path.join(tmpdir(self), "trimmed.json")
        with open(path, "w") as f:
            json.dump(config, f)
        self.assertNotIn("RPU", get_determinator(path))
        self.assertIn("RPU", get_determinator(CONFIG))


@needs_config
class TestGetManufacturer(unittest.TestCase):
    def detect(self, dev_type, value):
        dev = FakeDevice(read_str=[value])
        vendor = get_manufacturer(dev_type, dev, CONFIG)
        return vendor, dev

    def test_reads_the_register_the_device_type_names(self):
        _, dev = self.detect("PSU_PMM", "Delta")
        self.assertEqual(dev.calls_of("read_str"), [("read_str", 8, 8, 0)])

        _, dev = self.detect("ORV3_PSU", "12345678DE000000")
        self.assertEqual(dev.calls_of("read_str"), [("read_str", 24, 16, 0)])

    def test_exact_match(self):
        self.assertEqual(self.detect("PSU_PMM", "Delta")[0], "delta")
        self.assertEqual(self.detect("PSU_PMM", "ARTESYN")[0], "artesyn")
        self.assertEqual(self.detect("BBU", "Panasonic")[0], "panasonic")
        # A CBU spells it in caps where a PSU PMM does not.
        self.assertEqual(self.detect("CBU", "DELTA")[0], "delta")
        self.assertIsNone(self.detect("CBU", "Delta")[0])

    def test_exact_match_is_case_sensitive(self):
        self.assertIsNone(self.detect("PSU_PMM", "delta")[0])
        self.assertIsNone(self.detect("PSU_PMM", "DELTA")[0])

    def test_exact_match_ignores_register_padding(self):
        self.assertEqual(self.detect("PSU_PMM", "Delta\x00\x00\x00")[0], "delta")
        self.assertEqual(self.detect("BBU", "Panasonic       ")[0], "panasonic")

    def test_a_manufacturer_may_have_several_names(self):
        # A Panasonic BBU can report itself as ARTESYN.
        self.assertEqual(self.detect("ORV3_BBU", "ARTESYN")[0], "panasonic")
        self.assertEqual(self.detect("ORV3_BBU", "Panasonic")[0], "panasonic")

    def test_regex_match_on_the_serial_number(self):
        # Characters 9-10 of PSU_MFR_Serial carry the vendor code.
        self.assertEqual(self.detect("PSU", "12345678AE901234")[0], "artesyn")
        self.assertEqual(self.detect("PSU", "12345678DE901234")[0], "delta")
        self.assertEqual(self.detect("ORV3_PSU", "12345678AE901234")[0], "artesyn")

    def test_regex_match_is_anchored_at_the_serial_number_start(self):
        # "DE" anywhere else in the serial is not a vendor code.
        self.assertIsNone(self.detect("PSU", "1234DE7890123456")[0])
        self.assertIsNone(self.detect("PSU", "123456789DE12345")[0])

    def test_regex_match_needs_the_leading_characters(self):
        # A backend which strips leading padding would break this.
        self.assertIsNone(self.detect("PSU", "DE901234")[0])

    def test_unknown_value_has_no_manufacturer(self):
        self.assertIsNone(self.detect("PSU_PMM", "Acme")[0])
        self.assertIsNone(self.detect("ORV3_BBU", "")[0])

    def test_a_sole_manufacturer_matches_whatever_is_reported(self):
        # RPU2 is only ever a Cooler Master, whatever it reports.
        vendor, dev = self.detect("RPU2", "anything at all")
        self.assertEqual(vendor, "coolermaster")
        self.assertEqual(dev.calls_of("read_str"), [("read_str", 192, 20, 0)])

    def test_rpu_reports_no_name_at_all_when_it_is_a_delta(self):
        # Delta RPUs answer the name register with padding only.
        self.assertEqual(self.detect("RPU", "\x00\x00\x00\x00")[0], "delta")
        self.assertEqual(self.detect("RPU", "RDF040DSS5193E0")[0], "delta")
        self.assertEqual(self.detect("RPU", "L05T")[0], "quanta")

    def test_unknown_device_type(self):
        with self.assertRaises(KeyError):
            get_manufacturer("TOASTER", FakeDevice(read_str=["Delta"]), CONFIG)

    def detect_through_a_stale_override(self, dev_type, value):
        """
        Detect against an override which does not describe the RPU, as one
        left on the persistent partition by an older image would not, with
        the tree config standing in for the one shipped in the image.
        """
        with open(CONFIG) as f:
            config = json.load(f)
        del config["rpu"]
        override = os.path.join(tmpdir(self), "override-config.json")
        with open(override, "w") as f:
            json.dump(config, f)

        out = io.StringIO()
        with patch.object(manufacturers, "DEFAULT_CONFIG_PATH", CONFIG):
            with contextlib.redirect_stdout(out):
                vendor = get_manufacturer(
                    dev_type, FakeDevice(read_str=[value]), override
                )
        return vendor, out.getvalue()

    def test_a_stale_override_falls_back_to_the_default_config(self):
        vendor, out = self.detect_through_a_stale_override("RPU", "L05T")
        self.assertEqual(vendor, "quanta")
        self.assertIn("RPU is not in", out)

    def test_the_override_still_wins_for_what_it_does_describe(self):
        vendor, out = self.detect_through_a_stale_override("PSU_PMM", "Delta")
        self.assertEqual(vendor, "delta")
        self.assertEqual(out, "")

    def test_a_device_type_neither_config_knows_is_still_an_error(self):
        with self.assertRaises(KeyError):
            self.detect_through_a_stale_override("TOASTER", "Delta")

    def test_read_failures_are_not_swallowed(self):
        dev = FakeDevice(read_str=[ValueError("boom")])
        with self.assertRaises(ValueError):
            get_manufacturer("PSU_PMM", dev, CONFIG)


@needs_config
class TestDeterminatorTable(unittest.TestCase):
    def setUp(self):
        self.determinator = get_determinator(CONFIG)

    def test_every_entry_is_usable(self):
        for dev_type, config in self.determinator.items():
            with self.subTest(dev_type=dev_type):
                self.assertIsInstance(config["manufacturerDiscriminatorRegister"], int)
                self.assertGreater(config["manufacturerDiscriminatorLength"], 0)
                self.assertTrue(config["manufacturers"])

    def test_every_manufacturer_can_be_matched(self):
        for dev_type, config in self.determinator.items():
            for name, matcher in config["manufacturers"].items():
                with self.subTest(dev_type=dev_type, manufacturer=name):
                    self.assertTrue(
                        matcher.get("registerRegex") or matcher.get("registerValues"),
                        "no way to identify this manufacturer",
                    )

    def test_every_regex_compiles(self):
        import re

        for dev_type, config in self.determinator.items():
            for _name, matcher in config["manufacturers"].items():
                regex = matcher.get("registerRegex")
                if regex is None:
                    continue
                with self.subTest(dev_type=dev_type, pattern=regex):
                    re.compile(regex)

    def test_every_type_modbus_update_dispatches_on_is_known_here(self):
        # The config is what tells modbus-update.py which vendor a device
        # is, so the two tables have to line up.
        import test_modbus_update

        mu = test_modbus_update.load_modbus_update()
        for dev_type in mu.UPDATERS:
            with self.subTest(dev_type=dev_type):
                self.assertIn(dev_type, self.determinator)
        for dev_type in ("RPU", "RPU2"):
            self.assertIn(dev_type, self.determinator)

    def test_every_vendor_the_config_names_has_an_updater(self):
        # modbus-update.py may offer an updater for a vendor a device type
        # cannot be (the PMM types share one vendor table), but a vendor the
        # config can detect with no updater behind it is a gap.
        import test_modbus_update

        mu = test_modbus_update.load_modbus_update()
        for dev_type, (_, vendors) in mu.UPDATERS.items():
            for vendor in self.determinator[dev_type]["manufacturers"]:
                with self.subTest(dev_type=dev_type, vendor=vendor):
                    self.assertIn(vendor, vendors)


if __name__ == "__main__":
    unittest.main()
