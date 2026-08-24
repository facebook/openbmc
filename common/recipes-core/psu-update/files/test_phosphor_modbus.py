import io
import json
import os
import tempfile
import unittest
from unittest.mock import patch

import phosphor_modbus
from phosphor_modbus import (
    _device_config_from_interfaces,
    ConfigError,
    DeviceConfig,
    get_all_device_configs,
    get_containing_board,
    get_device_config,
    get_device_profile,
    get_line_settings,
    PhosphorModbusExclusion,
)

CONFIG_IFACE = phosphor_modbus.CONFIGURATION_IFACE_PREFIX + "BBU"


def quiet():
    """Silence the warnings these functions print to stderr"""
    return patch("sys.stderr", new=io.StringIO())


class TestDeviceConfig(unittest.TestCase):
    def config(self, **kwargs):
        args = {
            "name": "BBU_1_1",
            "address": 0x32,
            "serial_port": "ttyRS485-1",
            "types": ["BBU"],
            "board_path": "/xyz/openbmc_project/inventory/system/board/Board",
            "config_path": "/xyz/openbmc_project/inventory/system/board/Board/BBU_1_1",
        }
        args.update(kwargs)
        return DeviceConfig(**args)

    def test_a_bare_port_name_is_made_absolute(self):
        self.assertEqual(self.config().device_path, "/dev/ttyRS485-1")

    def test_an_absolute_port_is_left_alone(self):
        cfg = self.config(serial_port="/dev/serial/by-path/rs485")
        self.assertEqual(cfg.device_path, "/dev/serial/by-path/rs485")

    def test_parity_is_translated_to_the_pyserial_spelling(self):
        self.assertEqual(self.config(parity="None").parity_char, "N")
        self.assertEqual(self.config(parity="Even").parity_char, "E")
        self.assertEqual(self.config(parity="Odd").parity_char, "O")

    def test_unknown_parity_has_no_pyserial_spelling(self):
        self.assertIsNone(self.config(parity="Mark").parity_char)

    def test_parity_is_unknown_until_a_profile_provides_it(self):
        self.assertIsNone(self.config().parity)
        self.assertIsNone(self.config().parity_char)
        self.assertIsNone(self.config().baudrate)

    def test_str_reports_the_transport(self):
        text = str(self.config(baudrate=19200, parity="Even"))
        self.assertIn("BBU_1_1", text)
        self.assertIn("0x32", text)
        self.assertIn("/dev/ttyRS485-1", text)
        self.assertIn("19200", text)
        self.assertIn("Even", text)

    def test_str_marks_the_line_settings_it_does_not_know(self):
        self.assertIn("? ?", str(self.config()))


class TestGetDeviceProfile(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        patcher = patch.object(phosphor_modbus, "PROFILE_DIR", self.dir.name)
        patcher.start()
        self.addCleanup(patcher.stop)
        # The cache outlives a single lookup, so it has to be emptied
        # between tests.
        patcher = patch.object(phosphor_modbus, "_profiles", {})
        patcher.start()
        self.addCleanup(patcher.stop)

    def write(self, device_type, content):
        path = os.path.join(self.dir.name, device_type + ".json")
        with open(path, "w") as f:
            f.write(content)
        return path

    def test_reads_the_profile_of_a_type(self):
        self.write("BBU", json.dumps({"BaudRate": 19200, "Parity": "Even"}))
        self.assertEqual(
            get_device_profile("BBU"), {"BaudRate": 19200, "Parity": "Even"}
        )

    def test_a_missing_profile_is_not_fatal(self):
        with quiet() as err:
            self.assertIsNone(get_device_profile("Nope"))
        self.assertIn("WARNING", err.getvalue())

    def test_an_unparsable_profile_is_not_fatal(self):
        self.write("BBU", "{not json")
        with quiet():
            self.assertIsNone(get_device_profile("BBU"))

    def test_a_profile_which_is_not_an_object_is_rejected(self):
        self.write("BBU", "[1, 2]")
        with quiet():
            self.assertIsNone(get_device_profile("BBU"))

    def test_the_result_is_cached(self):
        path = self.write("BBU", json.dumps({"BaudRate": 19200}))
        self.assertEqual(get_device_profile("BBU"), {"BaudRate": 19200})
        os.unlink(path)
        self.assertEqual(get_device_profile("BBU"), {"BaudRate": 19200})

    def test_a_failed_read_is_cached_too(self):
        with quiet() as err:
            self.assertIsNone(get_device_profile("Nope"))
            self.assertIsNone(get_device_profile("Nope"))
        self.assertEqual(err.getvalue().count("WARNING"), 1)


class TestGetLineSettings(unittest.TestCase):
    def profiles(self, mapping):
        return patch.object(
            phosphor_modbus, "get_device_profile", side_effect=mapping.get
        )

    def test_takes_the_settings_from_the_profile(self):
        with self.profiles({"BBU": {"BaudRate": 19200, "Parity": "Even"}}):
            self.assertEqual(get_line_settings(["BBU"]), (19200, "Even"))

    def test_types_without_a_profile_are_skipped(self):
        with self.profiles({"BBU": {"BaudRate": 19200, "Parity": "Even"}}):
            self.assertEqual(get_line_settings(["Nope", "BBU"]), (19200, "Even"))

    def test_settings_may_come_from_different_profiles(self):
        with self.profiles({"A": {"BaudRate": 19200}, "B": {"Parity": "Odd"}}):
            self.assertEqual(get_line_settings(["A", "B"]), (19200, "Odd"))

    def test_no_profile_at_all(self):
        with self.profiles({}):
            self.assertEqual(get_line_settings(["A"]), (None, None))
        self.assertEqual(get_line_settings([]), (None, None))

    def test_disagreeing_profiles_keep_the_first_and_warn(self):
        with self.profiles({"A": {"BaudRate": 19200}, "B": {"BaudRate": 115200}}):
            with quiet() as err:
                self.assertEqual(get_line_settings(["A", "B"]), (19200, None))
        self.assertIn("BaudRate of B is 115200", err.getvalue())

    def test_agreeing_profiles_do_not_warn(self):
        with self.profiles({"A": {"Parity": "Even"}, "B": {"Parity": "Even"}}):
            with quiet() as err:
                self.assertEqual(get_line_settings(["A", "B"]), (None, "Even"))
        self.assertEqual(err.getvalue(), "")

    def test_baudrate_is_an_int_even_if_the_profile_spells_it_as_a_string(self):
        with self.profiles({"A": {"BaudRate": "19200"}}):
            self.assertEqual(get_line_settings(["A"]), (19200, None))


class TestGetContainingBoard(unittest.TestCase):
    BOARD = "/xyz/openbmc_project/inventory/system/board/Board"

    def test_follows_the_contained_by_association(self):
        associations = [
            ["powering", "powered_by", "/some/psu"],
            ["contained_by", "containing", self.BOARD],
        ]
        with patch.object(
            phosphor_modbus, "get_property", return_value=associations
        ) as get:
            self.assertEqual(get_containing_board("BBU_1_1"), self.BOARD)
        get.assert_called_once_with(
            phosphor_modbus.INVENTORY_SERVICE,
            phosphor_modbus.INVENTORY_CHASSIS_PATH + "/BBU_1_1",
            phosphor_modbus.ASSOCIATIONS_IFACE,
            "Associations",
        )

    def test_no_such_association(self):
        with patch.object(phosphor_modbus, "get_property", return_value=[]):
            with self.assertRaises(ConfigError):
                get_containing_board("BBU_1_1")

    def test_a_malformed_association_is_skipped(self):
        with patch.object(
            phosphor_modbus, "get_property", return_value=[["contained_by", "x"]]
        ):
            with self.assertRaises(ConfigError):
                get_containing_board("BBU_1_1")


class TestDeviceConfigFromInterfaces(unittest.TestCase):
    PATH = "/xyz/openbmc_project/inventory/system/board/Board/BBU_1_1"

    def build(self, interfaces, line_settings=(19200, "Even")):
        with patch.object(
            phosphor_modbus, "get_line_settings", return_value=line_settings
        ):
            return _device_config_from_interfaces(self.PATH, interfaces)

    def test_reads_the_transport_off_the_configuration_interface(self):
        cfg = self.build(
            {CONFIG_IFACE: {"Address": 50, "SerialPort": "ttyRS485-1", "Type": "BBU"}}
        )
        self.assertEqual(cfg.name, "BBU_1_1")
        self.assertEqual(cfg.address, 50)
        self.assertEqual(cfg.serial_port, "ttyRS485-1")
        self.assertEqual(cfg.types, ["BBU"])
        self.assertEqual(
            cfg.board_path, "/xyz/openbmc_project/inventory/system/board/Board"
        )
        self.assertEqual(cfg.config_path, self.PATH)
        self.assertEqual((cfg.baudrate, cfg.parity), (19200, "Even"))

    def test_the_interface_suffix_stands_in_for_a_missing_type(self):
        cfg = self.build({CONFIG_IFACE: {"Address": 50, "SerialPort": "ttyRS485-1"}})
        self.assertEqual(cfg.types, ["BBU"])

    def test_non_configuration_interfaces_are_ignored(self):
        cfg = self.build(
            {
                "xyz.openbmc_project.Inventory.Item": {"Present": True},
                CONFIG_IFACE: {"Address": 50, "SerialPort": "ttyRS485-1"},
            }
        )
        self.assertEqual(cfg.types, ["BBU"])

    def test_a_device_probed_as_several_part_numbers(self):
        # Same transport, two configuration interfaces: both types are
        # kept so their profiles are consulted.
        cfg = self.build(
            {
                CONFIG_IFACE
                + "_A": {
                    "Address": 50,
                    "SerialPort": "ttyRS485-1",
                    "Type": "BBU_A",
                },
                CONFIG_IFACE
                + "_B": {
                    "Address": 50,
                    "SerialPort": "ttyRS485-1",
                    "Type": "BBU_B",
                },
            }
        )
        self.assertEqual(cfg.types, ["BBU_A", "BBU_B"])
        self.assertEqual(cfg.address, 50)

    def test_interfaces_which_disagree_on_how_to_reach_the_device(self):
        with self.assertRaises(ConfigError) as ctx:
            self.build(
                {
                    CONFIG_IFACE + "_A": {"Address": 50, "SerialPort": "ttyRS485-1"},
                    CONFIG_IFACE + "_B": {"Address": 51, "SerialPort": "ttyRS485-1"},
                }
            )
        self.assertIn("conflicting configurations", str(ctx.exception))

    def test_an_interface_missing_the_transport_is_not_usable(self):
        for props in ({"Address": 50}, {"SerialPort": "ttyRS485-1"}, {}):
            with self.subTest(props=props):
                with self.assertRaises(ConfigError):
                    self.build({CONFIG_IFACE: props})

    def test_no_configuration_interface_at_all(self):
        with self.assertRaises(ConfigError):
            self.build({"xyz.openbmc_project.Inventory.Item": {"Present": True}})

    def test_an_address_which_is_not_a_number(self):
        with self.assertRaises(ValueError):
            self.build({CONFIG_IFACE: {"Address": "junk", "SerialPort": "tty"}})


class TestGetDeviceConfig(unittest.TestCase):
    BOARD = "/xyz/openbmc_project/inventory/system/board/Board"

    def test_reads_only_the_configuration_interfaces_of_the_device(self):
        props = {"Address": 50, "SerialPort": "ttyRS485-1", "Type": "BBU"}
        with patch.object(
            phosphor_modbus, "get_containing_board", return_value=self.BOARD
        ):
            with patch.object(
                phosphor_modbus,
                "get_interfaces",
                return_value=["org.freedesktop.DBus.Peer", CONFIG_IFACE],
            ):
                with patch.object(
                    phosphor_modbus, "get_all_properties", return_value=props
                ) as get_all:
                    with patch.object(
                        phosphor_modbus,
                        "get_line_settings",
                        return_value=(19200, "Even"),
                    ):
                        cfg = get_device_config("BBU_1_1")
        get_all.assert_called_once_with(
            phosphor_modbus.ENTITY_MANAGER_SERVICE,
            self.BOARD + "/BBU_1_1",
            CONFIG_IFACE,
        )
        self.assertEqual(cfg.address, 50)

    def test_rejects_a_name_which_is_not_an_inventory_name(self):
        for name in ("", "board/BBU_1_1", "/BBU_1_1"):
            with self.subTest(name=name):
                with self.assertRaises(ConfigError):
                    get_device_config(name)


class TestGetAllDeviceConfigs(unittest.TestCase):
    BOARD = "/xyz/openbmc_project/inventory/system/board/Board"

    def device(self, name, address, port="ttyRS485-1"):
        return (
            self.BOARD + "/" + name,
            {CONFIG_IFACE: {"Address": address, "SerialPort": port, "Type": "BBU"}},
        )

    def all_configs(self, objects):
        with patch.object(
            phosphor_modbus, "get_managed_objects", return_value=dict(objects)
        ):
            with patch.object(
                phosphor_modbus, "get_line_settings", return_value=(19200, "Even")
            ):
                with quiet() as err:
                    return get_all_device_configs(), err.getvalue()

    def test_keyed_by_inventory_name(self):
        configs, _ = self.all_configs(
            [self.device("BBU_1_1", 50), self.device("BBU_1_2", 51)]
        )
        self.assertEqual(sorted(configs), ["BBU_1_1", "BBU_1_2"])
        self.assertEqual(configs["BBU_1_2"].address, 51)

    def test_objects_which_are_not_modbus_devices_are_skipped(self):
        configs, _ = self.all_configs(
            [
                (self.BOARD, {"xyz.openbmc_project.Inventory.Item.Board": {}}),
                (
                    self.BOARD + "/Fan",
                    {phosphor_modbus.CONFIGURATION_IFACE_PREFIX + "Fan": {"Index": 1}},
                ),
                self.device("BBU_1_1", 50),
            ]
        )
        self.assertEqual(list(configs), ["BBU_1_1"])

    def test_one_broken_object_does_not_fail_the_enumeration(self):
        broken = (
            self.BOARD + "/BBU_1_2",
            {
                CONFIG_IFACE + "_A": {"Address": 1, "SerialPort": "ttyRS485-1"},
                CONFIG_IFACE + "_B": {"Address": 2, "SerialPort": "ttyRS485-1"},
            },
        )
        configs, err = self.all_configs([self.device("BBU_1_1", 50), broken])
        self.assertEqual(list(configs), ["BBU_1_1"])
        self.assertIn("Skipping", err)

    def test_a_name_configured_twice_keeps_the_first(self):
        first = self.device("BBU_1_1", 50)
        second = ("/xyz/openbmc_project/inventory/system/board/Other/BBU_1_1", first[1])
        configs, err = self.all_configs([first, second])
        self.assertEqual(configs["BBU_1_1"].config_path, first[0])
        self.assertIn("already configured", err)

    def test_no_devices_at_all(self):
        configs, _ = self.all_configs([])
        self.assertEqual(configs, {})


class TestPhosphorModbusExclusion(unittest.TestCase):
    PORT = "/dev/ttyRS485-1"
    PATH = phosphor_modbus.PORT_NAMESPACE + "/ttyRS485_1"

    def setUp(self):
        self.exclusion = PhosphorModbusExclusion(self.PORT)
        patcher = patch.object(phosphor_modbus, "is_unit_running", return_value=True)
        self.is_unit_running = patcher.start()
        self.addCleanup(patcher.stop)
        patcher = patch.object(phosphor_modbus, "set_property")
        self.set_property = patcher.start()
        self.addCleanup(patcher.stop)

    def objects(self, *paths):
        return patch.object(
            phosphor_modbus,
            "get_managed_objects",
            return_value={p: {phosphor_modbus.PORT_IFACE: {}} for p in paths},
        )

    def test_the_object_path_spells_the_tty_without_dashes(self):
        # D-Bus object paths cannot contain '-'.
        self.assertEqual(self.exclusion.tty_name, "ttyRS485_1")
        self.assertEqual(PhosphorModbusExclusion("/dev/ttyS0").tty_name, "ttyS0")

    def test_get_port_paths_only_matches_our_tty(self):
        other = phosphor_modbus.PORT_NAMESPACE + "/ttyRS485_2"
        with self.objects(self.PATH, other):
            self.assertEqual(self.exclusion.get_port_paths(), [self.PATH])

    def test_get_port_paths_skips_objects_without_the_port_interface(self):
        with patch.object(
            phosphor_modbus,
            "get_managed_objects",
            return_value={self.PATH: {"org.freedesktop.DBus.Peer": {}}},
        ):
            self.assertEqual(self.exclusion.get_port_paths(), [])

    def test_stop_disables_monitoring_and_start_puts_it_back(self):
        with self.objects(self.PATH):
            self.assertTrue(self.exclusion.stop())
        self.set_property.assert_called_once_with(
            phosphor_modbus.MODBUS_SERVICE,
            self.PATH,
            phosphor_modbus.PORT_IFACE,
            phosphor_modbus.MONITORING_ENABLED,
            "b",
            "false",
        )
        self.assertEqual(self.exclusion.changed_paths, [self.PATH])

        self.set_property.reset_mock()
        self.exclusion.start()
        self.set_property.assert_called_once_with(
            phosphor_modbus.MODBUS_SERVICE,
            self.PATH,
            phosphor_modbus.PORT_IFACE,
            phosphor_modbus.MONITORING_ENABLED,
            "b",
            "true",
        )
        self.assertEqual(self.exclusion.changed_paths, [])

    def test_stop_is_idempotent(self):
        with self.objects(self.PATH):
            self.assertTrue(self.exclusion.stop())
            self.assertTrue(self.exclusion.stop())
        self.assertEqual(self.set_property.call_count, 1)

    def test_nothing_to_stop_when_the_service_is_down(self):
        self.is_unit_running.return_value = False
        with self.objects(self.PATH):
            self.assertFalse(self.exclusion.stop())
        self.set_property.assert_not_called()

    def test_nothing_to_stop_when_the_service_does_not_own_the_port(self):
        with self.objects():
            self.assertFalse(self.exclusion.stop())
        self.set_property.assert_not_called()

    def test_a_port_which_cannot_be_enumerated_is_not_stopped(self):
        with patch.object(
            phosphor_modbus, "get_managed_objects", side_effect=ConfigError("boom")
        ):
            with quiet():
                self.assertFalse(self.exclusion.stop())

    def test_a_failed_stop_re_enables_what_it_already_disabled(self):
        # Half-disabled monitoring would leave the service polling some
        # ports and not others once we are done.
        second = phosphor_modbus.PORT_NAMESPACE + "/bus1/ttyRS485_1"
        paths = [self.PATH, second]
        with patch.object(self.exclusion, "get_port_paths", return_value=paths):
            self.set_property.side_effect = [None, ConfigError("denied"), None]
            with quiet() as err:
                self.assertFalse(self.exclusion.stop())
        self.assertIn("Could not disable", err.getvalue())
        self.assertEqual(self.set_property.call_args_list[-1][0][5], "true")
        self.assertEqual(self.exclusion.changed_paths, [])

    def test_start_reports_but_survives_a_failure(self):
        with self.objects(self.PATH):
            self.exclusion.stop()
        self.set_property.side_effect = ConfigError("denied")
        with quiet() as err:
            self.exclusion.start()
        self.assertIn("Could not enable", err.getvalue())
        self.assertEqual(self.exclusion.changed_paths, [])

    def test_enable_monitoring_does_not_need_a_matching_stop(self):
        # Recovery path after an update was killed mid-flight.
        with self.objects(self.PATH):
            self.assertEqual(self.exclusion.enable_monitoring(), [self.PATH])
        self.assertEqual(self.set_property.call_args[0][5], "true")

    def test_get_monitoring_reports_each_port(self):
        with self.objects(self.PATH):
            with patch.object(
                phosphor_modbus, "get_property", return_value=True
            ) as get:
                self.assertEqual(self.exclusion.get_monitoring(), {self.PATH: True})
        get.assert_called_once_with(
            phosphor_modbus.MODBUS_SERVICE,
            self.PATH,
            phosphor_modbus.PORT_IFACE,
            phosphor_modbus.MONITORING_ENABLED,
        )


class TestMonitoringMain(unittest.TestCase):
    PORT = "/dev/ttyRS485-1"
    PATH = phosphor_modbus.PORT_NAMESPACE + "/ttyRS485_1"

    class Args:
        __slots__ = ("stop_monitoring", "start_monitoring", "show_monitoring")

        def __init__(self, stop=None, start=None, show=None):
            self.stop_monitoring = stop
            self.start_monitoring = start
            self.show_monitoring = show

    def run_main(self, args, **exclusion):
        with patch.object(phosphor_modbus, "is_unit_running", return_value=True):
            with patch.object(phosphor_modbus, "PhosphorModbusExclusion") as cls:
                for name, value in exclusion.items():
                    setattr(cls.return_value, name, value)
                with patch("sys.stdout", new=io.StringIO()) as out:
                    with quiet():
                        rc = phosphor_modbus.monitoring_main(args)
        return rc, out.getvalue()

    def test_show_reports_the_state_of_each_port(self):
        rc, out = self.run_main(
            self.Args(show=self.PORT), get_monitoring=lambda: {self.PATH: True}
        )
        self.assertEqual(rc, 0)
        self.assertIn("%s: MonitoringEnabled = True" % self.PATH, out)

    def test_show_fails_when_no_port_object_matches(self):
        rc, out = self.run_main(self.Args(show=self.PORT), get_monitoring=lambda: {})
        self.assertEqual(rc, 1)
        self.assertIn("No port object", out)

    def test_stop_reports_what_it_disabled(self):
        rc, out = self.run_main(
            self.Args(stop=self.PORT),
            stop=lambda: True,
            changed_paths=[self.PATH],
            get_monitoring=lambda: {self.PATH: False},
        )
        self.assertEqual(rc, 0)
        self.assertIn("Disabled monitoring on: " + self.PATH, out)

    def test_stop_says_so_when_there_was_nothing_to_disable(self):
        rc, out = self.run_main(
            self.Args(stop=self.PORT),
            stop=lambda: False,
            get_monitoring=lambda: {self.PATH: True},
        )
        self.assertIn("Did not disable monitoring", out)

    def test_a_dbus_failure_is_reported_as_an_error(self):
        def boom():
            raise ConfigError("no bus")

        rc, _ = self.run_main(self.Args(show=self.PORT), get_monitoring=boom)
        self.assertEqual(rc, 1)


class TestMain(unittest.TestCase):
    def run_main(self, argv):
        with patch("sys.argv", ["phosphor_modbus.py"] + argv):
            with patch("sys.stdout", new=io.StringIO()) as out:
                with quiet() as err:
                    try:
                        rc = phosphor_modbus.main()
                    except SystemExit as e:
                        rc = e.code
        return rc, out.getvalue(), err.getvalue()

    def test_an_inventory_name_or_all_is_required(self):
        rc, _, _ = self.run_main([])
        self.assertEqual(rc, 2)

    def test_all_does_not_take_a_name(self):
        rc, _, _ = self.run_main(["--all", "BBU_1_1"])
        self.assertEqual(rc, 2)

    def test_prints_the_transport_of_one_device(self):
        cfg = DeviceConfig(
            name="BBU_1_1",
            address=0x32,
            serial_port="ttyRS485-1",
            types=["BBU"],
            board_path="/board",
            config_path="/board/BBU_1_1",
            baudrate=19200,
            parity="Even",
        )
        with patch.object(phosphor_modbus, "get_device_config", return_value=cfg):
            rc, out, _ = self.run_main(["BBU_1_1"])
        self.assertEqual(rc, 0)
        self.assertIn("Address:     0x32 (50)", out)
        self.assertIn("Serial Port: /dev/ttyRS485-1", out)
        self.assertIn("Parity:      Even (E)", out)

    def test_json_output_includes_the_derived_fields(self):
        cfg = DeviceConfig(
            name="BBU_1_1",
            address=0x32,
            serial_port="ttyRS485-1",
            types=["BBU"],
            board_path="/board",
            config_path="/board/BBU_1_1",
            baudrate=19200,
            parity="Even",
        )
        with patch.object(phosphor_modbus, "get_device_config", return_value=cfg):
            rc, out, _ = self.run_main(["--json", "BBU_1_1"])
        parsed = json.loads(out)
        self.assertEqual(parsed["device_path"], "/dev/ttyRS485-1")
        self.assertEqual(parsed["parity_char"], "E")

    def test_all_with_no_configured_device_fails(self):
        with patch.object(phosphor_modbus, "get_all_device_configs", return_value={}):
            rc, _, err = self.run_main(["--all"])
        self.assertEqual(rc, 1)
        self.assertIn("No configured modbus device", err)

    def test_a_device_which_is_not_configured_fails(self):
        with patch.object(
            phosphor_modbus, "get_device_config", side_effect=ConfigError("nope")
        ):
            rc, _, err = self.run_main(["BBU_9_9"])
        self.assertEqual(rc, 1)
        self.assertIn("ERROR: nope", err)


if __name__ == "__main__":
    unittest.main()
