import unittest
from unittest import mock

from .. import fw_json_upgrader as fw_up


class FwJsonTest(unittest.TestCase):
    def setUp(self):
        self.fw = fw_up.FwJson(binarypath=mock)

    @mock.patch("os.listdir")
    def test__get_json_filename(self, mocked_listdir):
        """
        Test *_fw_versions.json is in cwd
        """
        mocked_listdir.return_value = ["blah", "blah2.json"]
        with self.assertRaises(Exception):
            fw_up.FwJson()._get_json_filename(endpattern="_fw_versions.json")

    @mock.patch("os.listdir")
    def test__fetch_fw_manifest_json(self, mocked_listdir):
        """
        Test *_fw_manifest.json is in cwd
        """
        mocked_listdir.return_value = ["blah", "blah2.json"]
        with self.assertRaises(Exception):
            fw_up.FwJson()._get_json_filename(endpattern="_fw_manifest.json")

    @mock.patch("os.listdir")
    def test__fetch_fw_versions_with_manifest_json(self, mocked_listdir):
        """
        Test *_fw_versions.json is in cwd but testing for manifest
        """
        mocked_listdir.return_value = ["blah", "blah2_fw_versions.json"]
        with self.assertRaises(Exception):
            fw_up.FwJson()._get_json_filename(endpattern="_fw_manifest.json")

    @mock.patch("os.listdir")
    def test__fetch_fw_manifest_with_versions_json(self, mocked_listdir):
        """
        Test *_fw_manifest.json is in cwd but testing for versions
        """
        mocked_listdir.return_value = ["blah", "blah2_fw_manifest.json"]
        with self.assertRaises(Exception):
            fw_up.FwJson()._get_json_filename(endpattern="_fw_versions.json")

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
            fw_up.FwJson()._get_merged_version_data(fake1, fake2)

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
        result = fw_up.FwJson(
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
            fw_up.FwJson(binarypath=mock, fw_entity=["component4"])._filter_fw_entities(
                merged_data
            )

    @mock.patch(
        f"tools.fw-upgrade.fw_json_upgrader.FwJson._load_versions_json_file"  # noqa: E999
    )
    @mock.patch(f"tools.fw-upgrade.fw_json_upgrader.FwJson._load_manifest_json_file")
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
        f"tools.fw-upgrade.fw_json_upgrader.FwEntityUpgrader._get_entity_list_string_in_json"  # noqa: B950
    )
    @mock.patch(
        f"tools.fw-upgrade.fw_json_upgrader.FwEntityUpgrader._is_version_set_in_json"
    )
    def test__is_entity_upgrade_needed_no_version(
        self, mock_is_version_set_in_json, mock_get_entity_list_string_in_json
    ):
        mock_is_version_set_in_json.return_value = False
        mock_get_entity_list_string_in_json.return_value = None
        self.assertTrue(self.fwentityupgrader._is_entity_upgrade_needed())

    @mock.patch(
        f"tools.fw-upgrade.fw_json_upgrader.FwEntityUpgrader._get_entity_list_string_in_json"  # noqa: B950
    )
    @mock.patch(
        f"tools.fw-upgrade.fw_json_upgrader.FwEntityUpgrader._is_version_set_in_json"
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
        f"tools.fw-upgrade.fw_json_upgrader.FwEntityUpgrader._is_version_set_in_json"
    )
    @mock.patch(f"subprocess.check_output")
    @mock.patch(
        f"tools.fw-upgrade.fw_json_upgrader.FwEntityUpgrader._compare_current_and_package_versions"  # noqa: B950
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
