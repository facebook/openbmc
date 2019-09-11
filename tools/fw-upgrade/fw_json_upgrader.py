#!/usr/bin/python3
import argparse
import collections
import glob
import hashlib
import json
import logging
import mmap
import os
import re
import subprocess
import sys
import tarfile
import time
from enum import Enum
from typing import Any, Dict, Iterable, List, Optional


# Keys from fw_version_schema.json
UFW_VERSION = "version"
UFW_NAME = "filename"
UFW_HASH = "hash"
UFW_HASH_VALUE = "hash_value"

# Keys from fw_manifest_schema.json
UFW_GET_VERSION = "get_version"
UFW_CMD = "upgrade_cmd"
UFW_PRIORITY = "priority"
UFW_REBOOT_REQD = "reboot_required"
UFW_ENTITY_INSTANCE = "entities"
UFW_POST_ACTION = "post_action"
UFW_CONDITION = "condition"
UFW_CONTINUE_ON_ERROR = "continue_on_error"

# Other constants
# Wait between upgrads - in secs
WAIT_BTWN_UPGRADE = 5
DEFAULT_UPGRADABLE_ENTITIES = "all"


# =============================================================================
# Enumerations
# =============================================================================
class ResetMode(Enum):
    USERVER_RESET = "yes"
    HARD_RESET = "hard"
    NO_RESET = "no"


class UpgradeMode(Enum):
    UPGRADE = "yes"
    NO_UPGRADE = "no"
    FORCE_UPGRADE = "Forced Mode Only"


class HashType(Enum):
    SHA1SUM = "sha1sum"
    MD5SUM = "md5sum"


class UpgradeState(Enum):
    NONE = 100
    SUCCESS = 1
    FAIL = 0


# =============================================================================
# Exceptions
# =============================================================================


class FwBaseException(Exception):
    pass


class FwJsonExceptionMultipleFiles(FwBaseException):
    pass


class FwJsonExceptionMissingFiles(FwBaseException):
    pass


class FwJsonExceptionMismatchEntities(FwBaseException):
    pass


class FwUpgraderUnexpectedJsonKey(FwBaseException):
    pass


class FwUpgraderMissingJsonKey(FwBaseException):
    pass


class FwUpgraderUnexpectedFileHash(FwBaseException):
    pass


class FwUpgraderMissingFileHash(FwBaseException):
    pass


class FwUpgraderFailedUpgrade(FwBaseException):
    pass


# =============================================================================
# Core Classes
# =============================================================================


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


class FwEntityUpgrader(object):
    _REGEX_VERSION_PATTERN = r"^[v]?([0-9]*)\.([0-9]*)$"
    _REGEX_STRING_PATTERN = r"^[0-9a-zA-Z]*$"

    def __init__(
        self,
        fw_entity: str,
        fw_info,  # type: Dict[str, Any]
        binarypath: str,
        stop_on_error: bool = False,
        dryrun: bool = False,
        forced_upgrade: bool = False,
    ):
        """
        Parameters:
        fw_entity: fw entity to upgrade
        fw_info: info about fw entity
        stop_on_error: param to track if upgrade should stop on any failure
        dryrun: echo upgrade commands, but do not run them
        forced_upgrade: Even if versions are same, should we need to force an upgrade
        """
        self._fw_entity = fw_entity  # type: str
        # Using Any for expecting a str, list of str
        self._fw_info = fw_info  # type: Dict[str, Any]
        self._stop_on_error = stop_on_error  # type: bool
        self._dryrun = dryrun  # type: bool
        self._forced_upgrade = forced_upgrade  # type: bool
        self._cwd = binarypath  # type: str

    # =========================================================================
    # Helper APIs for running though a upgrade
    # =========================================================================

    # Step 1
    def _get_entities_list(
        self, entities: Iterable[str] = None
    ) -> Iterable[Optional[str]]:
        """
        Given a entity list string like
        ["1", "2", "3", "4", "5", "6", "7", "8"]
        or
        ["left"]
        verify entities are in the expected format and return list
        """
        if not entities:
            # Default to 1 value so we iterate at least once
            return [None]
        for item in entities:
            matched = re.findall(self._REGEX_STRING_PATTERN, item)
            if not matched:
                msg = "Malformed string in list : {} ".format(entities)
                raise FwUpgraderUnexpectedJsonKey(msg)
        return entities

    # Step 2.1
    def _is_file_md5sum_match(self, filename: str, expected_md5sum: str) -> bool:
        """
        Given a filename, verify if its md5sum matches the expected value
        """
        h_md5 = hashlib.md5()
        with open(filename, "r+b") as f, mmap.mmap(f.fileno(), 0) as mm:
            # mypi doesnt like mmap but it works!
            h_md5.update(mm)  # type: ignore
            file_md5 = h_md5.hexdigest()
        if expected_md5sum != file_md5:
            logging.info("FILE MD5 : " + file_md5)
            logging.info("JSON MD5 : " + expected_md5sum)
        return expected_md5sum == file_md5

    # Step 2.2
    def _is_file_sha1sum_match(self, filename: str, expected_sha1sum: str) -> bool:
        """
        Given a filename, verify if its sha1sum matches the expected value
        """
        h_sha1 = hashlib.sha1()
        with open(filename, "r+b") as f, mmap.mmap(f.fileno(), 0) as mm:
            # mypi doesnt like mmap but it works!
            h_sha1.update(mm)  # type: ignore
            file_sha1 = h_sha1.hexdigest()
        if expected_sha1sum != file_sha1:
            logging.info("FILE SHA1 : " + file_sha1)
            logging.info("JSON SHA1 : " + expected_sha1sum)
        return expected_sha1sum == file_sha1

    # Step 2
    def _verify_item_binary(self, filename: str) -> None:
        """
        Verify if fw_entity's file has valid SHASUM or MD5SUM
        """
        if not UFW_HASH and UFW_HASH_VALUE in self._fw_info:
            msg = "Hash missing for fw entity {}".format(self._fw_entity)
            raise FwUpgraderMissingFileHash(msg)

        if self._fw_info[UFW_HASH] == HashType.SHA1SUM.value:
            if not self._is_file_sha1sum_match(filename, self._fw_info[UFW_HASH_VALUE]):
                msg = "sha1sum not matching for fw entity {}".format(self._fw_entity)
                raise FwUpgraderUnexpectedFileHash(msg)
        elif self._fw_info[UFW_HASH] == HashType.MD5SUM.value:
            if not self._is_file_md5sum_match(filename, self._fw_info[UFW_HASH_VALUE]):
                msg = "md5sum not matching for fw entity {}".format(self._fw_entity)
                raise FwUpgraderUnexpectedFileHash(msg)
        else:
            msg = "Unknown hash for {} {}".format(
                self._fw_entity, self._fw_info[UFW_HASH]
            )
            raise FwUpgraderUnexpectedFileHash(msg)

    # Step 3.1
    def _compare_current_and_package_versions(self, current_ver, package_ver) -> bool:
        """
        # Version string can be random. So we try the following strategy
        # Case 1. If it is x.xx format or vx.xx, parse it as major / minor,
        # and compare accordingly
        # Case 2. Otherwise, we just compare it as string
        """
        need_to_upgrade = False
        re_pattern = self._REGEX_VERSION_PATTERN
        re_result = re.search(re_pattern, current_ver)
        if re_result:
            # Case 1
            logging.debug("Current Version : {}".format(current_ver))
            logging.debug("JSON Version    : {}".format(package_ver))
            cur_major = int(re_result.group(1))
            cur_minor = int(re_result.group(2))
            pkg_re = re.search(re_pattern, package_ver)
            if pkg_re:
                pkg_major = int(pkg_re.group(1))
                pkg_minor = int(pkg_re.group(2))
                logging.debug(
                    "FW Entity   : {}(version number comparison)".format(
                        self._fw_entity
                    )
                )
                logging.debug("Package : {}.{}".format(str(pkg_major), str(pkg_minor)))
                logging.debug("Current : {}.{}".format(str(cur_major), str(cur_minor)))
                need_to_upgrade = (pkg_major, pkg_minor) > (cur_major, cur_minor)
            else:
                logging.warning("Couldnt parse version, defaulting to upgrade")
                need_to_upgrade = True
        else:
            # Case 2
            logging.debug("FW Entity   : {}(string comparison)".format(self._fw_entity))
            logging.debug("Package : {}".format(str(package_ver)))
            logging.debug("Current : {}".format(str(current_ver)))
            need_to_upgrade = package_ver > current_ver
        return need_to_upgrade

    # Step 3
    def _is_entity_upgrade_needed(self, instance_specifier=None) -> bool:
        """
        Given one component check is firmware upgrade needed or not based on the
        version
        """
        need_to_upgrade = False
        entities = self._get_entity_list_string_in_json()
        instance_list = self._get_entities_list(entities)

        for instance in instance_list:
            # If instance specifier is set, deal with that instance only
            if instance_specifier and instance_specifier != instance:
                pass
            else:
                if self._is_version_set_in_json():
                    cmd_to_execute = self._fw_info[UFW_GET_VERSION]
                    if entities:
                        cmd_to_execute = cmd_to_execute.format(entity=instance)
                    current_version = (
                        subprocess.check_output(cmd_to_execute, shell=True)  # noqa P204
                        .decode()
                        .strip()
                    )
                    need_to_upgrade = self._compare_current_and_package_versions(
                        current_version, self._fw_info[UFW_VERSION]
                    )
                else:
                    logging.debug(
                        "=== No way to check version. Defaulting to upgrade..."
                    )
                    need_to_upgrade = True
        if need_to_upgrade:
            logging.debug("fw_entity needs upgrade")
        else:
            logging.debug("fw_entity does not need upgrade")
        return need_to_upgrade

    def _run_cmd_on_oob(self, cmd_to_execute: str) -> int:
        """
        Run command on OOB
        """
        logging.info("=== Running command : {}".format(cmd_to_execute))
        if self._dryrun:
            cmd_to_execute = "echo dryrun: " + cmd_to_execute
        try:
            subprocess.check_call(cmd_to_execute, shell=True, stderr=subprocess.STDOUT)
            return 0
        except subprocess.CalledProcessError as e:
            logging.info("Exception {} occured when running command".format(e))
            return e.returncode

    # Step 4
    def _upgrade_executor(self, filename: str, instance_specifier=None):
        """
        upgrade executor for an entity
        inputs:
        instance_specifier: identify a specific instance like pim '1' etc
        filename: binary that will be used for flashing
        """
        logging.info(
            "=== Upgrading...{} dryrun={}".format(self._fw_entity, self._dryrun)
        )
        instance_successful = True

        cmd_to_execute = self._fw_info[UFW_CMD]
        if not instance_specifier:
            cmd_to_execute = cmd_to_execute.format(filename=filename)
        else:
            cmd_to_execute = cmd_to_execute.format(
                filename=filename, entity=instance_specifier
            )

        return_code = self._run_cmd_on_oob(cmd_to_execute)
        instance_successful = return_code == 0
        if not instance_successful:
            logging.info(
                "=== Error occured with return code : {}".format(str(return_code))
            )
        return return_code, instance_successful

    # Step 5
    def _run_post_upgrade_action(self, item_successful: UpgradeState):
        """
        Inputs:
        item_successful: Status of all entities in list upgraded successfully or not
        """
        if self._is_post_action_set_in_json():
            if not item_successful == UpgradeState.FAIL:
                logging.info(
                    "=== Will not run post action command,"
                    "as one or more instances of this fw_entity failed to upgrade"
                )
            elif item_successful == UpgradeState.SUCCESS:
                post_action = self._fw_info[UFW_POST_ACTION]
                if self._dryrun:
                    post_action = "echo dryrun: " + post_action
                logging.debug(
                    "=== Running post action command : {}".format(post_action)
                )
                subprocess.check_output(post_action, shell=True)  # noqa p204
            else:
                logging.warn("=== Could not determine upgrade state")

    # ========================================================================
    # API publically accessible for upgrading entity
    # ========================================================================

    def upgrade_entity(self) -> List:
        """
        Upgrade fw_entity in json, if fw_entity has multiple entities(instances)
        then verify and upgrade them all.
        Steps:
        1) Get Instance list
        2) Verify if binary file md5/sha match
        3) Verify if upgrade is needed
        4) Perform upgrade if:
        4.1) Condition set in json passes
        4.2) Lower version and needs upgrade
        5) Post upgrade actions
        """
        failure_list = []  # Track instance(s) failed
        entity_upgrade = UpgradeState.NONE
        item_priority = self._fw_info[UFW_PRIORITY]
        logging.info(
            "\n\n=== Upgrading fw_entity : {} (Priority : {}) ===".format(
                self._fw_entity, str(item_priority)
            )
        )
        instance_list = self._get_entities_list(self._get_entity_list_string_in_json())
        filename = os.path.join(self._cwd, self._fw_info[UFW_NAME])
        self._verify_item_binary(filename)

        for instance in instance_list:
            logging.info("\n=== Entity : {} dryrun={}".format(instance, self._dryrun))
            start_time = time.time()
            if self._is_entity_upgrade_needed(instance_specifier=instance):
                # Check if "condition" field is set. If so, check that condition
                if (
                    self._is_condition_set_in_json(instance_specifier=instance)
                    or self._forced_upgrade
                ):
                    return_code, instance_successful = self._upgrade_executor(
                        filename, instance_specifier=instance
                    )
                    if instance_successful:
                        entity_upgrade = UpgradeState.SUCCESS
                    else:
                        # Even if 1 instance failed to upgrade, deem whole entity
                        # failed
                        entity_upgrade = UpgradeState.FAIL
                        end_time = time.time()
                        msg = "<{} instance : {}>".format(self._fw_entity, instance)
                        failure_list.append(msg)
                        logging.info(
                            "=== Time Elapsed : {}sec".format(end_time - start_time)
                        )
                        # If stop_on_error is set, stop no matter what
                        # Otherwise, check if continue_on_error field is set in JSON
                        if self._is_continue_on_error_set_in_json():
                            logging.info(
                                "=== Error occured with return code : {}".format(
                                    str(return_code)
                                )
                            )
                            logging.info(
                                "=== Continuing, as continue_on_error"
                                " is set in JSON field"
                            )
                        else:
                            # fail hard
                            raise FwUpgraderFailedUpgrade(
                                "Return code : {}".format(str(return_code))
                            )
                else:
                    logging.info(
                        "=== Condition not met. Will not upgrade {} instance {}".format(
                            self._fw_entity, instance
                        )
                    )
                end_time = time.time()
                logging.info(
                    "=== Time Elapsed : {}sec".format(
                        str(int(end_time) - int(start_time))
                    )
                )
            else:
                logging.info("=== Already up to date")
        self._run_post_upgrade_action(entity_upgrade)
        return failure_list

    # =============================================================================
    # APIs to check/get data from json dict
    # =============================================================================

    def _get_entity_list_string_in_json(self) -> Optional[List[str]]:
        """
        FW entity can provide a "entities" list optionally. When that is provided
        honor it
        """
        return self._fw_info.get(UFW_ENTITY_INSTANCE, None)

    def _is_condition_set_in_json(self, instance_specifier=None) -> bool:
        if UFW_CONDITION not in self._fw_info:
            return True
        cmd_to_execute = self._fw_info[UFW_CONDITION]
        if instance_specifier:
            cmd_to_execute = cmd_to_execute.format(entity=instance_specifier)
        try:
            subprocess.check_output(cmd_to_execute, shell=True)  # noqa p204
            return True
        except subprocess.CalledProcessError as e:
            logging.info("Exception {} occured when running command".format(e))
            return False

    def _is_continue_on_error_set_in_json(self) -> bool:
        if not self._stop_on_error and (
            UFW_CONTINUE_ON_ERROR in self._fw_info
            and self._fw_info[UFW_CONTINUE_ON_ERROR]
        ):
            return True
        return False

    def _is_version_set_in_json(self) -> bool:
        return UFW_GET_VERSION in self._fw_info

    def _is_post_action_set_in_json(self) -> bool:
        return UFW_POST_ACTION in self._fw_info


class FwUpgrader(object):
    _POWER_RESET = "wedge_power.sh reset"
    _POWER_RESET_HARD = "wedge_power.sh reset -s"

    def __init__(
        self,
        json: Dict,
        binarypath: str,
        stop_on_error: bool = False,
        dryrun: bool = False,
        forced_upgrade: bool = False,
        reset: ResetMode = ResetMode.NO_RESET,
    ):
        self._ordered_json = json  # type: Dict
        self._stop_on_error = stop_on_error  # type: bool
        self._dryrun = dryrun  # type: bool
        self._forced_upgrade = forced_upgrade  # type: bool
        self._reset = reset  # type: ResetMode
        self._cwd = binarypath  # type: str

    # TODO: Return should be Union[str, List[str], None]
    def _get_fw_info_for_entity(self, fw_entity: str) -> Dict[str, Any]:
        if fw_entity in self._ordered_json:
            return self._ordered_json[fw_entity]
        else:
            msg = "FW entity {} not found in JSON table!".format(fw_entity)
            raise FwUpgraderMissingJsonKey(msg)

    def _entity_upgrade_needed(self, fw_entity: str) -> bool:
        """
        Given one entity check is firmware upgrade needed or not
        """
        return FwEntityUpgrader(
            fw_entity,
            self._get_fw_info_for_entity(fw_entity),
            binarypath=self._cwd,
            stop_on_error=self._stop_on_error,
            dryrun=self._dryrun,
            forced_upgrade=self._forced_upgrade,
        )._is_entity_upgrade_needed()

    # =========================================================================
    # API publically accessible for upgrading all entities
    # =========================================================================

    def print_if_upgrade_needed(self) -> None:
        upgrade_needed = False
        for fw_entity in self._ordered_json:
            if self._entity_upgrade_needed(fw_entity):
                upgrade_needed = True
                break  # As long as there is atleast 1 component to upgrade return
        logging.info("Upgrade Needed : " + "YES" if upgrade_needed else "NO")

    def run_upgrade(self) -> bool:
        """
        Upgrade each upgradable fw_entity in json
        """
        all_successful = True  # type: bool
        entity_failure_list = []  # type: List[str]
        fail_list = []  # type: List[str]

        for fw_entity in self._ordered_json:
            # fw_entity could have multiple instances, hence fetch list
            entity_failure_list = FwEntityUpgrader(
                fw_entity,
                self._get_fw_info_for_entity(fw_entity),
                binarypath=self._cwd,
                stop_on_error=self._stop_on_error,
                dryrun=self._dryrun,
                forced_upgrade=self._forced_upgrade,
            ).upgrade_entity()
            if len(entity_failure_list):
                all_successful = False
                fail_list.extend(entity_failure_list)
        if not all_successful:
            raise FwUpgraderFailedUpgrade(",".join(fail_list))
        return all_successful

    def reboot_as_needed(self) -> None:
        if self._reset == ResetMode.USERVER_RESET.value:
            subprocess.check_output(self._POWER_RESET, shell=True)  # noqa p204
        elif self._reset == ResetMode.HARD_RESET.value:
            subprocess.check_output(self._POWER_RESET_HARD, shell=True)  # noqa p204


class Main(object):
    tool_help = "JSON Firmware Upgrader"

    def __init__(self):
        self.zipfile = None  # type: str
        self.fw_entity_items = None  # type: List[str]
        self.check_only = False  # type: bool
        self.list_only = False  # type: bool
        self.reset = ResetMode.NO_RESET  # type: ResetMode
        self.verbose = False  # type: bool
        self.forced_upgrade = False  # type: bool
        self.stop_on_error = True  # type: bool
        self.dryrun = False  # type: bool

    def _fw_arg_parser(self):
        parser = argparse.ArgumentParser(description=self.tool_help)
        parser.add_argument(
            "-z",
            "--zipfile",
            dest="zipfile",
            action="store",
            default=None,
            help="zip file to use, that contains the whole package",
        )
        parser.add_argument(
            "-i",
            "--entities",
            dest="entities",
            action="store",
            default=DEFAULT_UPGRADABLE_ENTITIES,
            help="Comma separated list of entities to upgrade. All, by default",
        )
        parser.add_argument(
            "-r",
            "--reset",
            dest="reset",
            action="store",
            default=ResetMode.NO_RESET.value,
            choices=[
                ResetMode.USERVER_RESET.value,
                ResetMode.HARD_RESET.value,
                ResetMode.NO_RESET.value,
            ],
        )
        parser.add_argument(
            "-v",
            "--verbose",
            action="store_true",
            dest="verbose",
            default=False,
            help="Verbose Output",
        )
        parser.add_argument(
            "-s",
            "--stop_on_error",
            action="store_true",
            dest="stop_on_error",
            default=True,
            help="Stop on failure, even when continue_on_error is set on JSON file",
        )
        parser.add_argument(
            "-c",
            "--check_only",
            action="store_true",
            dest="check_only",
            default=False,
            help="Do not upgrade and only check if upgrade needed",
        )
        parser.add_argument(
            "-l",
            "--list_only",
            action="store_true",
            dest="list_only",
            default=False,
            help="List all supported items in JSON, and exit",
        )
        parser.add_argument(
            "-d",
            "--dryrun",
            action="store_true",
            dest="dryrun",
            default=False,
            help="Dry-run mode. Just printout commands, and not launch upgrade",
        )
        parser.add_argument(
            "-f",
            "--forced",
            action="store_true",
            dest="forced_upgrade",
            default=False,
            help="Force upgrade, even when upgrade condition is not met",
        )

        return parser.parse_args()

    def _validate_args(self, args) -> None:
        if args.zipfile:
            self.zipfile = args.zipfile
            self._unpack_zipfile()
        if args.entities:
            self.fw_entity_items = args.entities.split(",")
        if args.reset:
            self.reset = args.reset
        if args.check_only:
            self.check_only = args.check_only
        if args.dryrun:
            self.dryrun = args.dryrun
        if args.forced_upgrade:
            self.forced_upgrade = args.forced_upgrade
        if args.list_only:
            self.list_only = args.list_only
        if args.stop_on_error:
            self.stop_on_error = args.stop_on_error
        if args.verbose:
            self.verbose = args.verbose

    def _dump_args(self) -> None:
        print("")
        print("=================================================")
        print("===  JSON based firmware upgrader : Settings  ===")
        print("=================================================")
        print(
            "  Zipfile to use          :  " + (self.zipfile if self.zipfile else "None")
        )
        print("  Items to upgrade        :  " + "".join(self.fw_entity_items))
        print("  Check only (no upgrade) :  " + ("yes" if self.check_only else "no"))
        print("  List  only (no upgrade) :  " + ("yes" if self.list_only else "no"))
        print("  Reboot after success    :  " + self.reset)
        print("  Verbose                 :  " + ("yes" if self.verbose else "no"))
        print("  Stop on error           :  " + ("yes" if self.stop_on_error else "no"))
        print("  Dry run mode            :  " + ("yes" if self.dryrun else "no"))
        print("=================================================")
        print("")

    def _unpack_zipfile(self) -> None:
        logging.debug("=== Unzipping file : " + self.zipfile + " ===")
        with tarfile.open(self.zipfile, "r:bz2") as tf:
            tf.extractall(".")
        logging.info("=== Finished unzipping " + self.zipfile + " ===")

    def _set_log_level(self) -> None:
        level = logging.INFO
        if self.verbose:
            level = logging.DEBUG
        logging.basicConfig(stream=sys.stdout, level=level)

    def run(self):
        args = self._fw_arg_parser()
        self._validate_args(args)
        binarypath = os.path.dirname(os.path.abspath(__file__))
        self._set_log_level()
        if self.verbose:
            self._dump_args()

        fw_struct_obj = FwJson(binarypath, self.fw_entity_items)
        ord_json = fw_struct_obj.get_priority_ordered_json()

        if self.list_only:
            fw_struct_obj.print_fw_entity_list(ord_json)
            exit(0)

        fw_upgrader_obj = FwUpgrader(
            ord_json,
            binarypath=binarypath,
            stop_on_error=self.stop_on_error,
            dryrun=self.dryrun,
            forced_upgrade=self.forced_upgrade,
            reset=self.reset,
        )
        if self.check_only:
            fw_upgrader_obj.print_if_upgrade_needed()
            exit(0)

        # Check version and upgrade one by one
        rc = fw_upgrader_obj.run_upgrade()
        logging.info("=== Finished upgrading firmware")

        # Reboot if needed
        fw_upgrader_obj.reboot_as_needed()

        if not rc:
            logging.info("=== Upgraded with failures")
            exit(1)


if __name__ == "__main__":
    Main().run()
