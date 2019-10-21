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

import tools.fw_upgrade.fw_json as fw_json


class FwJsonTest(unittest.TestCase):
    def setUp(self):
        self.fw = fw_json.FwJson(binarypath=mock)

    @mock.patch("os.listdir")
    def test__get_json_filename(self, mocked_listdir):
        """
        Test *_fw_versions.json is in cwd
        """
        mocked_listdir.return_value = ["blah", "blah2.json"]
        with self.assertRaises(Exception):
            fw_json.FwJson()._get_json_filename(endpattern="_fw_versions.json")

    @mock.patch("os.listdir")
    def test__fetch_fw_manifest_json(self, mocked_listdir):
        """
        Test *_fw_manifest.json is in cwd
        """
        mocked_listdir.return_value = ["blah", "blah2.json"]
        with self.assertRaises(Exception):
            fw_json.FwJson()._get_json_filename(endpattern="_fw_manifest.json")

    @mock.patch("os.listdir")
    def test__fetch_fw_versions_with_manifest_json(self, mocked_listdir):
        """
        Test *_fw_versions.json is in cwd but testing for manifest
        """
        mocked_listdir.return_value = ["blah", "blah2_fw_versions.json"]
        with self.assertRaises(Exception):
            fw_json.FwJson()._get_json_filename(endpattern="_fw_manifest.json")

    @mock.patch("os.listdir")
    def test__fetch_fw_manifest_with_versions_json(self, mocked_listdir):
        """
        Test *_fw_manifest.json is in cwd but testing for versions
        """
        mocked_listdir.return_value = ["blah", "blah2_fw_manifest.json"]
        with self.assertRaises(Exception):
            fw_json.FwJson()._get_json_filename(endpattern="_fw_versions.json")

    def test__get_merged_version_data_missing_entity(self):
        fake1 = {
            "component1": {"name": "foo", "version": "NA", "hash_value": "blahblah"},
            "component2": {"name": "foo2", "version": "any", "hash_value": "blahblah"},
        }

        fake2 = {
            "component1": {
                "entities": ["1", "2", "3", "4", "5", "6", "7", "8"],
                "get_version": "NA",
            },
            "component3": {
                "entities": ["1", "2", "3", "4", "5", "6", "7", "8"],
                "get_version": "NA",
            },
        }
        with self.assertRaises(Exception):
            fw_json.FwJson()._get_merged_version_data(fake1, fake2)

    def test__filter_fw_entities(self):
        fake1 = {
            "component1": {"name": "foo", "version": "NA", "hash_value": "blahblah"},
            "component2": {"name": "foo2", "version": "any", "hash_value": "blahblah"},
            "component3": {"name": "foo2", "version": "any", "hash_value": "blahblah"},
        }

        fake2 = {
            "component1": {
                "entities": ["1", "2", "3", "4", "5", "6", "7", "8"],
                "get_version": "NA",
            },
            "component2": {
                "get_version": "blah blah",
                "priority": "4",
                "upgrade_cmd": "blah blah blah",
            },
            "component3": {
                "entities": ["1", "2", "3", "4", "5", "6", "7", "8"],
                "get_version": "NA",
            },
        }
        merged_data = self.fw._get_merged_version_data(fake1, fake2)
        result = fw_json.FwJson(
            binarypath=mock, fw_entity=["component2", "component3"]
        )._filter_fw_entities(merged_data)
        self.assertNotIn("component1", result)
        self.assertIn("component2", result)
        self.assertIn("component3", result)

    def test__filter_fw_entities_no_entity(self):
        fake1 = {
            "component1": {"name": "foo", "version": "NA", "hash_value": "blahblah"},
            "component2": {"name": "foo2", "version": "any", "hash_value": "blahblah"},
            "component3": {"name": "foo2", "version": "any", "hash_value": "blahblah"},
        }

        fake2 = {
            "component1": {
                "entities": ["1", "2", "3", "4", "5", "6", "7", "8"],
                "get_version": "NA",
            },
            "component2": {
                "get_version": "blah blah",
                "priority": "4",
                "upgrade_cmd": "blah blah blah",
            },
            "component3": {
                "entities": ["1", "2", "3", "4", "5", "6", "7", "8"],
                "get_version": "NA",
            },
        }
        merged_data = self.fw._get_merged_version_data(fake1, fake2)
        with self.assertRaises(Exception):
            fw_json.FwJson(
                binarypath=mock, fw_entity=["component4"]
            )._filter_fw_entities(merged_data)

    @mock.patch(
        f"tools.fw_upgrade.fw_json.FwJson._load_versions_json_file"  # noqa: E999
    )
    @mock.patch(f"tools.fw_upgrade.fw_json.FwJson._load_manifest_json_file")
    def test__get_priority_unordered_json(
        self, mock_load_versions_json_file, mock_load_manifest_json_file
    ):
        fake1 = {
            "component1": {"name": "foo", "version": "NA", "hash_value": "blahblah"},
            "component2": {"name": "foo2", "version": "any", "hash_value": "blahblah"},
        }

        fake2 = {
            "component1": {
                "entities": ["1", "2", "3", "4", "5", "6", "7", "8"],
                "get_version": "NA",
            },
            "component2": {
                "get_version": "blah blah",
                "priority": "4",
                "upgrade_cmd": "blah blah blah",
            },
        }
        mock_load_versions_json_file.return_value = fake1
        mock_load_manifest_json_file.return_value = fake2
        unordered_json = self.fw._get_priority_unordered_json()
        self.assertIn("name", unordered_json["component1"])
        self.assertIn("version", unordered_json["component1"])
        self.assertIn("hash_value", unordered_json["component1"])
        self.assertIn("entities", unordered_json["component1"])
        self.assertIn("get_version", unordered_json["component1"])

        self.assertIn("name", unordered_json["component2"])
        self.assertIn("version", unordered_json["component2"])
        self.assertIn("hash_value", unordered_json["component2"])
        self.assertIn("priority", unordered_json["component2"])
        self.assertIn("get_version", unordered_json["component2"])
        self.assertIn("upgrade_cmd", unordered_json["component2"])
