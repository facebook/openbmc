import unittest

import manufacturers
from manufacturers import device_manufactor_determinator, get_manufacturer, normalize
from test_mocks import FakeDevice


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


class TestGetManufacturer(unittest.TestCase):
    def detect(self, dev_type, value):
        dev = FakeDevice(read_str=[value])
        vendor = get_manufacturer(dev_type, dev)
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
        self.assertEqual(self.detect("CBU", "Delta")[0], "delta")

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

    def test_a_sole_manufacturer_needs_no_matcher(self):
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
            get_manufacturer("TOASTER", FakeDevice(read_str=["Delta"]))

    def test_read_failures_are_not_swallowed(self):
        dev = FakeDevice(read_str=[ValueError("boom")])
        with self.assertRaises(ValueError):
            get_manufacturer("PSU_PMM", dev)


class TestDeterminatorTable(unittest.TestCase):
    def test_every_entry_is_usable(self):
        for dev_type, config in device_manufactor_determinator.items():
            with self.subTest(dev_type=dev_type):
                self.assertIsInstance(config["register"], int)
                self.assertGreater(config["length"], 0)
                self.assertTrue(config["manufacturers"])

    def test_every_regex_compiles(self):
        import re

        for dev_type, config in device_manufactor_determinator.items():
            if not config.get("isRegex", False):
                continue
            for patterns in config["manufacturers"].values():
                for pattern in patterns:
                    with self.subTest(dev_type=dev_type, pattern=pattern):
                        re.compile(pattern)

    def test_every_type_modbus_update_dispatches_on_is_known_here(self):
        # manufacturers.py is what tells modbus-update.py which vendor a
        # device is, so the two tables have to line up.
        import test_modbus_update

        mu = test_modbus_update.load_modbus_update()
        for dev_type in mu.UPDATERS:
            with self.subTest(dev_type=dev_type):
                self.assertIn(dev_type, device_manufactor_determinator)
        for dev_type in ("RPU", "RPU2"):
            self.assertIn(dev_type, device_manufactor_determinator)

    def test_module_exposes_the_table_under_its_public_name(self):
        self.assertIs(
            manufacturers.device_manufactor_determinator,
            device_manufactor_determinator,
        )


if __name__ == "__main__":
    unittest.main()
