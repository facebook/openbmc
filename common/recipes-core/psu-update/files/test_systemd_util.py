import json
import subprocess
import unittest
from unittest.mock import patch

import systemd_util
from systemd_util import (
    _busctl_call,
    _unwrap,
    ConfigError,
    get_all_properties,
    get_interfaces,
    get_managed_objects,
    get_property,
    get_subtree_paths,
    is_unit_running,
    set_property,
)


def reply(data, rc=0, stderr=b""):
    """A busctl --json=short reply of a method returning `data`"""
    out = b"" if data is None else json.dumps(data).encode()
    return subprocess.CompletedProcess([], rc, stdout=out, stderr=stderr)


class TestUnwrap(unittest.TestCase):
    def test_strips_a_variant_wrapper(self):
        self.assertEqual(_unwrap({"type": "s", "data": "active"}), "active")

    def test_strips_nested_wrappers(self):
        self.assertEqual(
            _unwrap({"type": "v", "data": {"type": "u", "data": 7}}),
            7,
        )

    def test_recurses_into_lists_and_dicts(self):
        self.assertEqual(
            _unwrap(
                {
                    "type": "a{sv}",
                    "data": [{"Address": {"type": "u", "data": 50}}],
                }
            ),
            [{"Address": 50}],
        )

    def test_leaves_a_dict_which_is_not_a_wrapper_alone(self):
        # "data" alongside other keys is a property named data, not a
        # busctl wrapper.
        value = {"type": "s", "data": "x", "other": 1}
        self.assertEqual(_unwrap(value), value)
        self.assertEqual(_unwrap({"data": "x"}), "x")

    def test_leaves_scalars_alone(self):
        for value in ("s", 1, True, None, 1.5):
            self.assertEqual(_unwrap(value), value)


class TestBusctlCall(unittest.TestCase):
    def test_builds_the_command_line(self):
        with patch.object(subprocess, "run", return_value=reply(None)) as run:
            _busctl_call("svc", "/path", "iface", "Method")
        cmd = run.call_args[0][0]
        self.assertEqual(
            cmd,
            [
                systemd_util.BUSCTL,
                "--json=short",
                "call",
                "svc",
                "/path",
                "iface",
                "Method",
            ],
        )
        self.assertEqual(run.call_args[1]["timeout"], systemd_util.BUSCTL_TIMEOUT)

    def test_appends_the_signature_and_stringified_arguments(self):
        with patch.object(subprocess, "run", return_value=reply(None)) as run:
            _busctl_call("svc", "/path", "iface", "Method", "sb", "name", True)
        self.assertEqual(run.call_args[0][0][-3:], ["sb", "name", "True"])

    def test_unwraps_the_reply(self):
        with patch.object(
            subprocess, "run", return_value=reply({"type": "s", "data": ["v"]})
        ):
            self.assertEqual(_busctl_call("s", "/p", "i", "M"), ["v"])

    def test_no_output_means_no_return_values(self):
        with patch.object(subprocess, "run", return_value=reply(None)):
            self.assertEqual(_busctl_call("s", "/p", "i", "M"), [])

    def test_a_failed_call_carries_the_stderr(self):
        with patch.object(
            subprocess, "run", return_value=reply(None, rc=1, stderr=b"No such unit\n")
        ):
            with self.assertRaises(ConfigError) as ctx:
                _busctl_call("s", "/p", "i", "M")
        self.assertIn("No such unit", str(ctx.exception))
        self.assertIn("i.M on /p", str(ctx.exception))

    def test_a_timeout_is_a_config_error(self):
        with patch.object(
            subprocess, "run", side_effect=subprocess.TimeoutExpired("busctl", 10)
        ):
            with self.assertRaises(ConfigError) as ctx:
                _busctl_call("s", "/p", "i", "M")
        self.assertIn("timed out", str(ctx.exception))

    def test_unparsable_output_is_a_config_error(self):
        broken = subprocess.CompletedProcess([], 0, stdout=b"not json", stderr=b"")
        with patch.object(subprocess, "run", return_value=broken):
            with self.assertRaises(ConfigError):
                _busctl_call("s", "/p", "i", "M")

    def test_undecodable_output_does_not_raise_unicode_errors(self):
        broken = subprocess.CompletedProcess([], 1, stdout=b"", stderr=b"\xff\xfe")
        with patch.object(subprocess, "run", return_value=broken):
            with self.assertRaises(ConfigError):
                _busctl_call("s", "/p", "i", "M")


class TestProperties(unittest.TestCase):
    def test_get_property_returns_the_variant_contents(self):
        with patch.object(
            systemd_util,
            "_busctl_call",
            return_value=["active"],
        ) as call:
            self.assertEqual(get_property("svc", "/p", "iface", "State"), "active")
        call.assert_called_once_with(
            "svc", "/p", systemd_util.PROPERTIES_IFACE, "Get", "ss", "iface", "State"
        )

    def test_get_property_with_no_reply(self):
        with patch.object(systemd_util, "_busctl_call", return_value=[]):
            with self.assertRaises(ConfigError):
                get_property("svc", "/p", "iface", "State")

    def test_get_all_properties(self):
        props = {"Address": 50, "SerialPort": "ttyS0"}
        with patch.object(systemd_util, "_busctl_call", return_value=[props]) as call:
            self.assertEqual(get_all_properties("svc", "/p", "iface"), props)
        call.assert_called_once_with(
            "svc", "/p", systemd_util.PROPERTIES_IFACE, "GetAll", "s", "iface"
        )

    def test_get_all_properties_of_an_interface_which_is_not_there(self):
        for ret in ([], ["not a dict"]):
            with self.subTest(ret=ret):
                with patch.object(systemd_util, "_busctl_call", return_value=ret):
                    with self.assertRaises(ConfigError):
                        get_all_properties("svc", "/p", "iface")

    def test_set_property_passes_the_value_as_a_variant(self):
        with patch.object(systemd_util, "_busctl_call", return_value=[]) as call:
            set_property("svc", "/p", "iface", "Enabled", "b", "false")
        call.assert_called_once_with(
            "svc",
            "/p",
            systemd_util.PROPERTIES_IFACE,
            "Set",
            "ssv",
            "iface",
            "Enabled",
            "b",
            "false",
        )

    def test_set_property_propagates_failures(self):
        with patch.object(
            systemd_util, "_busctl_call", side_effect=ConfigError("denied")
        ):
            with self.assertRaises(ConfigError):
                set_property("svc", "/p", "iface", "P", "b", "true")


class TestGetManagedObjects(unittest.TestCase):
    def test_returns_the_object_tree(self):
        objects = {"/p/1": {"iface": {"Address": 1}}}
        with patch.object(systemd_util, "_busctl_call", return_value=[objects]) as call:
            self.assertEqual(get_managed_objects("svc", "/p"), objects)
        call.assert_called_once_with(
            "svc", "/p", systemd_util.OBJECT_MANAGER_IFACE, "GetManagedObjects"
        )

    def test_a_service_with_no_object_manager(self):
        with patch.object(systemd_util, "_busctl_call", return_value=[]):
            with self.assertRaises(ConfigError):
                get_managed_objects("svc", "/p")


class TestGetSubTreePaths(unittest.TestCase):
    IFACE = "xyz.openbmc_project.Object.Enable"

    def test_asks_the_mapper_for_the_whole_subtree_by_default(self):
        paths = ["/p/1", "/p/2"]
        with patch.object(systemd_util, "_busctl_call", return_value=[paths]) as call:
            self.assertEqual(get_subtree_paths("/p", [self.IFACE]), paths)
        call.assert_called_once_with(
            systemd_util.OBJECT_MAPPER_SERVICE,
            systemd_util.OBJECT_MAPPER_PATH,
            systemd_util.OBJECT_MAPPER_IFACE,
            "GetSubTreePaths",
            "sias",
            "/p",
            0,
            1,
            self.IFACE,
        )

    def test_the_interface_array_is_a_count_then_its_entries(self):
        # busctl spells "as" as the number of entries followed by them.
        with patch.object(systemd_util, "_busctl_call", return_value=[[]]) as call:
            get_subtree_paths("/p", [self.IFACE, "iface2"], depth=2)
        self.assertEqual(call.call_args[0][5:], ("/p", 2, 2, self.IFACE, "iface2"))

    def test_a_subtree_the_mapper_does_not_know_is_a_config_error(self):
        for ret in ([], ["not a list"]):
            with self.subTest(ret=ret):
                with patch.object(systemd_util, "_busctl_call", return_value=ret):
                    with self.assertRaises(ConfigError):
                        get_subtree_paths("/p", [self.IFACE])


class TestGetInterfaces(unittest.TestCase):
    XML = (
        '<node name="/p">'
        '<interface name="org.freedesktop.DBus.Properties"/>'
        '<interface name="xyz.openbmc_project.Object.Enable">'
        '<property name="Enabled" type="b" access="readwrite"/>'
        "</interface>"
        '<node name="child"/>'
        "</node>"
    )

    def test_lists_the_interface_names(self):
        with patch.object(systemd_util, "_busctl_call", return_value=[self.XML]):
            self.assertEqual(
                get_interfaces("svc", "/p"),
                [
                    "org.freedesktop.DBus.Properties",
                    "xyz.openbmc_project.Object.Enable",
                ],
            )

    def test_ignores_child_nodes(self):
        with patch.object(
            systemd_util, "_busctl_call", return_value=['<node><node name="c"/></node>']
        ):
            self.assertEqual(get_interfaces("svc", "/p"), [])

    def test_bad_xml_is_a_config_error(self):
        with patch.object(systemd_util, "_busctl_call", return_value=["<node"]):
            with self.assertRaises(ConfigError):
                get_interfaces("svc", "/p")

    def test_no_reply_is_a_config_error(self):
        with patch.object(systemd_util, "_busctl_call", return_value=[]):
            with self.assertRaises(ConfigError):
                get_interfaces("svc", "/p")


class TestIsUnitRunning(unittest.TestCase):
    UNIT = "xyz.openbmc_project.ModbusRTU.service"
    PATH = "/org/freedesktop/systemd1/unit/modbus"

    def test_active_unit(self):
        with patch.object(
            systemd_util, "_busctl_call", return_value=[self.PATH]
        ) as call:
            with patch.object(
                systemd_util, "get_property", return_value="active"
            ) as get:
                self.assertTrue(is_unit_running(self.UNIT))
        call.assert_called_once_with(
            systemd_util.SYSTEMD_SERVICE,
            systemd_util.SYSTEMD_PATH,
            systemd_util.SYSTEMD_MANAGER_IFACE,
            "GetUnit",
            "s",
            self.UNIT,
        )
        get.assert_called_once_with(
            systemd_util.SYSTEMD_SERVICE,
            self.PATH,
            systemd_util.SYSTEMD_UNIT_IFACE,
            "ActiveState",
        )

    def test_inactive_unit(self):
        with patch.object(systemd_util, "_busctl_call", return_value=[self.PATH]):
            for state in ("inactive", "failed", "activating"):
                with self.subTest(state=state):
                    with patch.object(systemd_util, "get_property", return_value=state):
                        self.assertFalse(is_unit_running(self.UNIT))

    def test_a_unit_which_is_not_loaded_is_not_running(self):
        with patch.object(
            systemd_util, "_busctl_call", side_effect=ConfigError("No such unit")
        ):
            self.assertFalse(is_unit_running(self.UNIT))

    def test_no_unit_path_in_the_reply(self):
        with patch.object(systemd_util, "_busctl_call", return_value=[]):
            self.assertFalse(is_unit_running(self.UNIT))

    def test_an_unreadable_state_is_not_running(self):
        with patch.object(systemd_util, "_busctl_call", return_value=[self.PATH]):
            with patch.object(
                systemd_util, "get_property", side_effect=ConfigError("gone")
            ):
                self.assertFalse(is_unit_running(self.UNIT))


if __name__ == "__main__":
    unittest.main()
