import unittest
from unittest import mock

import test_mock_modules  # noqa: F401 # isort: skip
import rest_sensors


class LegacyAdapterNameTest(unittest.TestCase):
    def setUp(self):
        rest_sensors.legacy_adapter_name.cache_clear()

    def _name(self, platform, adapter):
        with mock.patch(
            "rest_sensors.rest_pal_legacy.pal_get_platform_name", return_value=platform
        ):
            return rest_sensors.legacy_adapter_name(adapter)

    def test_wedge100_restores_i2c_bus(self):
        for addr in ("1e78a100", "1e78a140", "1e78a340"):
            self.assertEqual(self._name("wedge100", addr + ".i2c"), addr + ".i2c-bus")

    def test_wedge100_leaves_non_aspeed_adapters(self):
        for adapter in ("ISA adapter", "i2c-7-mux (chan_id 0)"):
            self.assertEqual(self._name("wedge100", adapter), adapter)

    def test_other_platforms_unchanged(self):
        self.assertEqual(self._name("yamp", "1e78a100.i2c"), "1e78a100.i2c")
