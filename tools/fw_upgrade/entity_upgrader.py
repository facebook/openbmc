#!/usr/bin/python3
import hashlib
import logging
import mmap
import os
import re
import subprocess
import time
from exceptions import (
    FwUpgraderFailedUpgrade,
    FwUpgraderMissingFileHash,
    FwUpgraderMissingJsonKey,
    FwUpgraderUnexpectedFileHash,
    FwUpgraderUnexpectedJsonKey,
)
from typing import Any, Dict, Iterable, List, Optional

from constants import (
    UFW_CMD,
    UFW_CONDITION,
    UFW_CONTINUE_ON_ERROR,
    UFW_ENTITY_INSTANCE,
    UFW_GET_VERSION,
    UFW_HASH,
    UFW_HASH_VALUE,
    UFW_NAME,
    UFW_POST_ACTION,
    UFW_PRIORITY,
    UFW_VERSION,
    HashType,
    ResetMode,
    UpgradeState,
)


class FwEntityUpgrader(object):
    _REGEX_VERSION_PATTERN = r"^[v]?([0-9]*)\.([0-9]*)$"
    _REGEX_STRING_PATTERN = r"^[0-9a-zA-Z\-]+$"

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
                logging.debug(
                    "JSON Version (after parsing) : {}.{}".format(
                        str(pkg_major), str(pkg_minor)
                    )
                )
                logging.debug(
                    "Current Version (after parsing) : {}.{}".format(
                        str(cur_major), str(cur_minor)
                    )
                )
                need_to_upgrade = (pkg_major, pkg_minor) != (cur_major, cur_minor)

            else:
                logging.warning("Couldnt parse version, defaulting to upgrade")
                need_to_upgrade = True
        else:
            # Case 2
            logging.debug("FW Entity   : {}(string comparison)".format(self._fw_entity))
            logging.debug("JSON Version (after parsing) : {}".format(str(package_ver)))
            logging.debug(
                "Current Version (after parsing) : {}".format(str(current_ver))
            )
            need_to_upgrade = package_ver != current_ver
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
            if item_successful == UpgradeState.FAIL:
                logging.info(
                    "=== Will not run post action command,"
                    "as one or more instances of this fw_entity failed to upgrade"
                )
            elif item_successful == UpgradeState.SUCCESS:
                post_action = self._fw_info[UFW_POST_ACTION]
                if self._dryrun:
                    post_action = "echo dryrun: " + post_action
                logging.info(
                    "=== Running post action command : {}".format(post_action)
                )
                subprocess.check_output(post_action, shell=True)  # noqa p204
            else:
                logging.info("=== Upgrade skipped. Will not run post action")

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
            if (
                self._is_entity_upgrade_needed(instance_specifier=instance)
                or self._forced_upgrade
            ):
                # Check if "condition" field is set. If so, check that condition
                if self._is_condition_set_in_json(instance_specifier=instance):
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

    def is_any_upgrade_needed(self) -> bool:
        for fw_entity in self._ordered_json:
            if self._entity_upgrade_needed(fw_entity):
                return True
        return False

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
