#!/usr/bin/python3
import collections
import glob
import json
import logging
from exceptions import (
    FwJsonExceptionMismatchEntities,
    FwJsonExceptionMissingFiles,
    FwJsonExceptionMultipleFiles,
)
from typing import Dict, List

from constants import (
    DEFAULT_UPGRADABLE_ENTITIES,
    UFW_ENTITY_INSTANCE,
    UFW_PRIORITY,
    UpgradeMode,
)


class FwJson(object):
    _FW_VERSIONS_FILE_PATTERN = "_fw_versions.json"
    _FW_MANIFEST_FILE_PATTERN = "_fw_manifest.json"

    def __init__(self, binarypath: str, fw_entity: List[str] = ["all"]):  # noqa B006
        """
        Parameters:
        fw_entity(list of strings): list of fw entities, default to 'all'
        binarypath: Binaries needed for upgrade are in same location as the tool
        """
        self._fw_entity = fw_entity  # type: List[str]
        self._cwd = binarypath  # type: str

    def _get_json_filename(self, endpattern: str) -> str:
        """
        Get json file with expected file extension
        """
        path = self._cwd + "/*" + endpattern
        json_files = glob.glob(path)
        if len(json_files) > 1:
            msg = "Multiple JSON files found {} for pattern {}".format(
                json_files, endpattern
            )
            raise FwJsonExceptionMultipleFiles(msg)
        elif len(json_files) == 0:
            msg = "No JSON file with {} found in the directory {}".format(
                endpattern, self._cwd
            )
            raise FwJsonExceptionMissingFiles(msg)
        return json_files[0]

    def _load_json_file(self, json_filename: str) -> Dict:
        """
        Load json in parameter json_filename
        """
        logging.debug("=== Loading Json : {}".format(json_filename))
        with open(json_filename) as f:
            return json.load(f)

    def _load_versions_json_file(self) -> Dict:
        """
        Load json file with endpattern _FW_VERSIONS_FILE_PATTERN
        """
        versions_json_filename = self._get_json_filename(
            endpattern=self._FW_VERSIONS_FILE_PATTERN
        )
        logging.debug(
            "=== FW Package JSON File found : {}".format(versions_json_filename)
        )
        return self._load_json_file(versions_json_filename)

    def _load_manifest_json_file(self) -> Dict:
        """
        Load json file with endpattern _FW_MANIFEST_FILE_PATTERN
        """
        manifest_json_filename = self._get_json_filename(
            endpattern=self._FW_MANIFEST_FILE_PATTERN
        )
        logging.info("=== Manifest JSON File found : " + manifest_json_filename)
        return self._load_json_file(manifest_json_filename)

    def _get_merged_version_data(
        self, raw_version_data: Dict, raw_manifest_data: Dict
    ) -> Dict:
        """
        Given 2 json data, merge them to one
        """
        try:
            merged_version_data = {
                k: {**raw_version_data[k], **raw_manifest_data[k]}
                for k in raw_version_data
            }
        except KeyError as e:
            msg = "Entity {} doesn't have entry in manifest JSON".format(e)
            raise FwJsonExceptionMismatchEntities(msg)
        return merged_version_data

    def _filter_fw_entities(self, processed_data: Dict) -> Dict:
        """
        Filter for entities provided by user for upgrade
        """
        filtered_data = {}  # type: Dict
        for item in self._fw_entity:
            if item in processed_data:
                filtered_data[item] = processed_data[item]
            else:
                msg = "FW entity {} does not exist in filename {}".format(
                    item, self._FW_VERSIONS_FILE_PATTERN
                )
                raise FwJsonExceptionMismatchEntities(msg)
        logging.debug("=== Finished parsing JSON Files ===")
        return filtered_data

    def _get_priority_unordered_json(self) -> Dict:
        """
        Given 2 json:
         1) <name>_fw_versions.json
         3) <name>_fw_manifest.json
        Generate a json that determines versions and upgrade steps for each item
        """
        # Step1: Load FW Versions JSON file
        logging.debug(
            "=== Scanning For Firmware Versions JSON file in : {}".format(self._cwd)
        )
        raw_version_data = self._load_versions_json_file()

        # Step2: Load Manifest JSON file
        logging.debug(
            "=== Scanning For Firmware Manifest JSON file in : {}".format(self._cwd)
        )
        raw_manifest_data = self._load_manifest_json_file()

        # Step3: Now that we loaded both file, combine manifest information
        # into FW info dictionary
        processed_data = self._get_merged_version_data(
            raw_version_data, raw_manifest_data
        )
        logging.info("=== Finished combining FW Manifest with FW Versions ")

        # If upgrading all, we don't need to filter the entries by 'fw_entity' argument
        if self._fw_entity[0] == DEFAULT_UPGRADABLE_ENTITIES:
            logging.debug("=== All fw entity processed")
            return processed_data
        else:
            return self._filter_fw_entities(processed_data)

    # =========================================================================
    # APIs publically accessible for getting priority ordered json
    # =========================================================================

    def get_priority_ordered_json(self) -> Dict:
        """
        Returns an ordered (by priority) Dict of _unordered_fw_priority_json
        """
        _unordered_fw_priority_json = self._get_priority_unordered_json()
        orddict = collections.OrderedDict(
            sorted(
                _unordered_fw_priority_json.items(),
                key=lambda k_v: k_v[1][UFW_PRIORITY],
            )
        )
        return orddict

    def print_fw_entity_list(self, ordered_fw_priority_json: Dict) -> None:
        upgrade = UpgradeMode.UPGRADE

        print("")
        print(
            "==========================================================================================="  # noqa B950
        )
        print(
            "=== Name          Entity                                       Priority  Upgradable     ==="  # noqa B950
        )
        print(
            "==========================================================================================="  # noqa B950
        )
        for item, info in ordered_fw_priority_json.items():
            name = item.strip()
            entity = info.get(UFW_ENTITY_INSTANCE, "NA")
            priority = info[UFW_PRIORITY]
            print(
                "{: <16} {: <45} {: <16} {: <2}".format(
                    name, str(entity), priority, upgrade.value
                )
            )
        print(
            "==========================================================================================="  # noqa B950
        )
        print("")
