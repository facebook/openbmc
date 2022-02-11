#!/usr/bin/python3

# fmt: off
# isort:skip_file
# HACK:Inorder for unittests to know the location of fw_upgrade here is way for it.
# Ideally if yocto has a framework to integrate these unit tests this wouldnt
# be needed.
import inspect
import os
import sys
cmd_folder = (
    os.path.realpath(
        os.path.abspath(os.path.split(inspect.getfile(inspect.currentframe()))[0])
    )
).rstrip("_test")
if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)

import unittest
from unittest import mock

import tools.fw_upgrade.entity_upgrader as fw_up


class FwEntityUpgraderTest(unittest.TestCase):
    def setUp(self):
        self.fwentityupgrader = fw_up.FwEntityUpgrader(mock, mock, mock)

    def test__get_instance_entities_number_raises(self):
        with self.assertRaises(Exception):
            self.fwentityupgrader._get_entities_list(["1-9"])

    def test__get_entities_list_number(self):
        with self.assertRaises(Exception):
            self.fwentityupgrader._get_entities_list(["1", "2", "3"], ["1", "2", "3"])

    def test__get_entities_list_left_right(self):
        self.assertEqual(
            self.fwentityupgrader._get_entities_list(["left", "right"]),
            ["left", "right"],
        )

    def test__get_entities_list_default(self):
        self.assertEqual(self.fwentityupgrader._get_entities_list(), [None])

    def test__get_entities_list_invalid(self):
        with self.assertRaises(Exception):
            self.fwentityupgrader._get_entities_list(["+++"])

    def test__get_entities_list_no_comma(self):
        self.assertEqual(
            self.fwentityupgrader._get_entities_list(["leftright"]), ["leftright"]
        )

    def test__compare_current_and_package_versions(self):
        self.assertFalse(
            self.fwentityupgrader._compare_current_and_package_versions("v1.2", "v0.1")
        )
        self.assertTrue(
            self.fwentityupgrader._compare_current_and_package_versions("v0.2", "v4.4")
        )
        self.assertTrue(
            self.fwentityupgrader._compare_current_and_package_versions("4.5", "10.4")
        )
        self.assertTrue(
            self.fwentityupgrader._compare_current_and_package_versions("v4.5", "10.4")
        )
        self.assertFalse(
            self.fwentityupgrader._compare_current_and_package_versions("v14.5", "10.4")
        )
        self.assertTrue(
            self.fwentityupgrader._compare_current_and_package_versions("1.1", "4.5")
        )
        self.assertTrue(
            self.fwentityupgrader._compare_current_and_package_versions("4.5", "4.14")
        )

    @mock.patch(
        f"tools.fw_upgrade.entity_upgrader.FwEntityUpgrader._get_entity_list_string_in_json"  # noqa: B950
    )
    @mock.patch(
        f"tools.fw_upgrade.entity_upgrader.FwEntityUpgrader._is_version_set_in_json"
    )
    def test__is_entity_upgrade_needed_no_version(
        self, mock_is_version_set_in_json, mock_get_entity_list_string_in_json
    ):
        mock_is_version_set_in_json.return_value = False
        mock_get_entity_list_string_in_json.return_value = None
        self.assertTrue(self.fwentityupgrader._is_entity_upgrade_needed())

    @mock.patch(
        f"tools.fw_upgrade.entity_upgrader.FwEntityUpgrader._get_entity_list_string_in_json"  # noqa: B950
    )
    @mock.patch(
        f"tools.fw_upgrade.entity_upgrader.FwEntityUpgrader._is_version_set_in_json"
    )
    def test__is_entity_upgrade_needed_mismatch_specifier(
        self, mock_is_version_set_in_json, mock_get_entity_list_string_in_json
    ):
        mock_is_version_set_in_json.return_value = False
        mock_get_entity_list_string_in_json.return_value = ["1", "2", "3"]
        self.assertFalse(
            self.fwentityupgrader._is_entity_upgrade_needed(instance_specifier="5")
        )

    @mock.patch(
        f"tools.fw_upgrade.entity_upgrader.FwEntityUpgrader._is_version_set_in_json"
    )
    @mock.patch(f"subprocess.check_output")
    @mock.patch(
        f"tools.fw_upgrade.entity_upgrader.FwEntityUpgrader._compare_current_and_package_versions"  # noqa: B950
    )
    def test__is_entity_upgrade_needed_version_matched(
        self,
        mock_is_version_set_in_json,
        mock_sub_proc_output,
        mock_compare_current_and_package_versions,
    ):
        fake1 = {
            "name": "foo",
            "version": "NA",
            "hash_value": "blahblah",
            "entities": ["1", "2", "3", "4", "5", "6", "7", "8"],
            "priority": "2",
            "get_version": "blahblah",
        }
        self.fwentityupgrader._fw_info = fake1
        mock_is_version_set_in_json.return_value = True
        mock_sub_proc_output.return_value = "0.0".encode("utf-8")
        mock_compare_current_and_package_versions.return_value = True
        self.assertTrue(self.fwentityupgrader._is_entity_upgrade_needed())


class FwUpgraderTest(unittest.TestCase):
    def setUp(self):
        self.fwupgrader = fw_up.FwUpgrader(mock, mock)

    def test__get_fw_info_for_entity(self):
        fake1 = {
            "component1": {
                "name": "foo",
                "version": "NA",
                "hash_value": "blahblah",
                "entities": ["1", "2", "3", "4", "5", "6", "7", "8"],
                "priority": "2",
                "get_version": "NA",
            },
            "component2": {
                "name": "foo2",
                "version": "any",
                "hash_value": "blahblah",
                "get_version": "blah blah",
                "priority": "4",
                "upgrade_cmd": "blah blah blah",
            },
        }
        self.fwupgrader._ordered_json = fake1
        self.assertIn("name", self.fwupgrader._get_fw_info_for_entity("component1"))
        self.assertIn("version", self.fwupgrader._get_fw_info_for_entity("component1"))
        self.assertIn(
            "hash_value", self.fwupgrader._get_fw_info_for_entity("component1")
        )
        self.assertIn("entities", self.fwupgrader._get_fw_info_for_entity("component1"))
        self.assertIn(
            "get_version", self.fwupgrader._get_fw_info_for_entity("component1")
        )

    def test__get_fw_info_for_entity_invalid(self):
        fake1 = {
            "component1": {
                "name": "foo",
                "version": "NA",
                "hash_value": "blahblah",
                "get_version": "NA",
            },
            "component2": {
                "name": "foo2",
                "version": "any",
                "hash_value": "blahblah",
                "upgrade_cmd": "blah blah blah",
            },
        }
        self.fwupgrader._ordered_json = fake1
        with self.assertRaises(Exception):
            self.fwupgrader._get_fw_info_for_entity("component3")


# TODO:
# _is_condition_set_in_json()
