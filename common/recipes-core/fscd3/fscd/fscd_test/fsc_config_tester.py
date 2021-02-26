# Copyright 2004-present Facebook. All Rights Reserved.

from fsc_base_tester import BaseFscdUnitTest


DEFAULT_TRANSITIONAL = 70
DEFAULT_BOOST = 100
DEFAULT_RAMP_RATE = 10
DEFAULT_SENSOR_UTIL = "/usr/local/bin/sensor-util"
DEFAULT_FAN_UTIL = "/usr/local/bin/fan-util --get 2"
DEFAULT_FAN_UTIL_SET = "/usr/local/bin/fan-util --set %d 1"
DEFAULT_CHASSIS_INTRUSION_STATE = False


class FscdConfigUnitTest(BaseFscdUnitTest):
    # Following are config related tests:

    def test_config_params(self):
        """
        Tests config values against defaults
        """
        self.assertEqual(
            self.fscd_tester.transitional,
            DEFAULT_TRANSITIONAL,
            "Incorrect transitional Value",
        )
        self.assertEqual(self.fscd_tester.boost, DEFAULT_BOOST, "Incorrect boost Value")
        self.assertEqual(
            self.fscd_tester.ramp_rate, DEFAULT_RAMP_RATE, "Incorrect boost Value"
        )
        self.assertEqual(
            self.fscd_tester.chassis_intrusion,
            DEFAULT_CHASSIS_INTRUSION_STATE,
            "Incorrect chassis intrusion state for this config",
        )

    def test_zone_config(self):
        """
        Test zone object values against config values
        """
        for zone in self.fscd_tester.zones:
            self.assertEqual(int(zone.pwm_output[0]), 0, "Incorrect zone pwm=0")
            self.assertEqual(int(zone.pwm_output[1]), 1, "Incorrect zone pwm=1")
            self.assertNotEqual(int(zone.pwm_output[1]), 0, "Incorrect zone pwm=1")

    def test_fan_config(self):
        """
        Test fan object values against config values :
        name, read source, write source
        """
        self.assertEqual(
            int(self.fscd_tester.fans["0"].source.name), 0, "Incorrect fan pwm=0"
        )
        self.assertEqual(
            int(self.fscd_tester.fans["1"].source.name), 1, "Incorrect fan pwm=1"
        )
        self.assertNotEqual(
            int(self.fscd_tester.fans["1"].source.name), 0, "Incorrect fan pwm=0"
        )
        self.assertEqual(
            self.fscd_tester.fans["2"].source.read_source,
            DEFAULT_FAN_UTIL,
            "Incorrect fan read source",
        )
        self.assertEqual(
            self.fscd_tester.fans["1"].source.write_source,
            DEFAULT_FAN_UTIL_SET,
            "Incorrect fan write source",
        )
        self.assertNotEqual(
            self.fscd_tester.fans["1"].source.read_source,
            self.fscd_tester.fans["1"].source.write_source,
            "Read and write source are incorrect",
        )

    def test_fan_zone_config(self):
        """
        Tests fan object and zone object corelation
        """
        for zone in self.fscd_tester.zones:
            self.assertEqual(
                int(self.fscd_tester.fans["0"].source.name),
                int(zone.pwm_output[0]),
                "Incorrect fan pwm=0",
            )
            self.assertEqual(
                int(self.fscd_tester.fans["1"].source.name),
                int(zone.pwm_output[1]),
                "Incorrect fan pwm=1",
            )
            self.assertNotEqual(
                int(self.fscd_tester.fans["0"].source.name),
                int(zone.pwm_output[1]),
                "Incorrect fan pwm=1",
            )

    def test_profile_config(self):
        """
        Tests profile object values against config values :
        name, source type, source
        """
        self.assertEqual(
            self.fscd_tester.sensors["linear_dimm"].source.name,
            "linear_dimm",
            "Incorrect profile name",
        )
        self.assertEqual(
            self.fscd_tester.sensors["linear_cpu_margin"].source.name,
            "linear_cpu_margin",
            "Incorrect profile name",
        )
        self.assertNotEqual(
            self.fscd_tester.sensors["linear_dimm"].source.name,
            "linear_cpu_margin",
            "Incorrect profile name",
        )
        self.assertEqual(
            self.fscd_tester.sensors["linear_cpu_margin"].source.read_source,
            DEFAULT_SENSOR_UTIL,
            "Incorrect sensor read source",
        )
