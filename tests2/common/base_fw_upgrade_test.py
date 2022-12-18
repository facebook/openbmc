#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

import argparse
import collections
import copy
import os
import sys
import time
from abc import abstractmethod

import pexpect
from utils.cit_logger import Logger
from utils.ssh_util import OpenBMCSSHSession


# When tests are discovered upgrader is not yet installed,
# catch the import failure
SANDCASTLE_NEXUS = "SANDCASTLE_NEXUS"
DEV_SERVER_RESOURCE_PATH = (
    os.path.join(os.environ[SANDCASTLE_NEXUS], "fw_upgrade")
    if SANDCASTLE_NEXUS in os.environ
    else "/tmp/fw_upgrade"
)

try:
    # The upgrader need to be inside e.g. /tmp/fw_upgrade on dev server.
    sys.path.insert(0, DEV_SERVER_RESOURCE_PATH)
    import fw_json as fw_up
    from constants import (
        HashType,
        UFW_CMD,
        UFW_CONDITION,
        UFW_ENTITY_INSTANCE,
        UFW_GET_VERSION,
        UFW_HASH,
        UFW_HASH_VALUE,
        UFW_NAME,
        UFW_PRIORITY,
        UFW_VERSION,
    )
    from entity_upgrader import FwEntityUpgrader, FwUpgrader
except Exception:
    pass

# Global variables
G_VERBOSE = False


class BaseFwUpgradeTest(object):
    """
    On dev server
    1. copy directory tools/fw_upgrade to /tmp
    2. manually copy all firmware images to UUT bmc host: /mnt/data1/fw_upgrade/
       or specify binary path in test case.
    3. copy and modify <platform>_ufw_manifest.json and <platform>_ufw_version.json
       according to the platform directory to /tmp/fw_upgrade/.
    4. make sure that md5sum and filename in _ufw_version.json is the same
       as firmware image.

    On BMC
    1. static ip to interface to /mnt/data/etc/rc.local
        - add this line to file: ifconfig eth0 <ip>

    Test plan:
    Running cit external test command as below:
    1. force upgrade:
        python3 cit_runner.py
            --run-test "tests.wedge400.external_test_fw_upgrade.ForceFwUpgradeTest
            .test_external_firmware_upgrade" --external --bmc-host <bmc host ip address>
    2. normal upgrade:
        python3 cit_runner.py
            --run-test "tests.wedge400.external_test_fw_upgrade.FwUpgradeTest
            .test_external_firmware_upgrade" --external --bmc-host <bmc host ip address>
    """

    # Default settings which can be override in derived test class of each platform
    DEFAULT_UPGRADING_TIMEOUT = {
        "bic": 300,
        "bios": 1200,
        "fpga": 900,
        "scm": 300,
        "fcm": 300,
        "pwr": 300,
        "smb": 300,
    }
    EXPECTED_KEYWORD = [
        "closed by remote host",
        "not supported",
        "No EEPROM/flash",
        "FAIL!",
        "returned error",  # end of failed expected keyword
        "PASS!",
        "VERIFIED.",
        "content is identical",
        "bios succeeded",
        "bic succeeded",
    ]
    NUM_LAST_FAILED_EXPECTED_KEY = 4  # zero-based number 0...N
    DEFAULT_BMC_RECONNECT_TIMEOUT = 300  # BMC Booting timeout
    DEFAULT_SCM_BOOT_TIME = 30  # SCM startup time
    DEFAULT_COMMAND_EXEC_DELAY = 1  # Delay for UUT command handler
    DEFAULT_COMMAND_PROMTP_TIME_OUT = 10  # Waiting promtp timeout

    try:
        DEFAULT_POWER_RESET_CMD = FwUpgrader._POWER_RESET_HARD
    except Exception:
        pass

    USERVER_HOSTNAME = "DEFAULT"
    BMC_HOSTNAME = "DEFAULT"
    STOP_FSCD = "sv stop fscd"
    START_FSCD = "sv start fscd"
    STOP_WDT = "wdtcli stop"
    MAX_LINE_LEN = 83
    MAX_APPEND_LEN = 10

    def setUp(self):
        self.bmc_hostname = None
        self.hostname = None
        self.bmc_ssh_session = None
        self.upgrader_path = None
        self.remote_bin_path = None
        self.forced_upgrade = False
        self.skip_components = None
        self.num_last_failed_expected_key = self.NUM_LAST_FAILED_EXPECTED_KEY
        self.expected_keyword = self.EXPECTED_KEYWORD
        self.upgrading_timeout = self.DEFAULT_UPGRADING_TIMEOUT
        self.bmc_reconnect_timeout = self.DEFAULT_BMC_RECONNECT_TIMEOUT
        self.power_reset_cmd = self.DEFAULT_POWER_RESET_CMD
        self.scm_boot_time = self.DEFAULT_SCM_BOOT_TIME
        self.command_exec_delay = self.DEFAULT_COMMAND_EXEC_DELAY
        self.command_promtp_timeout = self.DEFAULT_COMMAND_PROMTP_TIME_OUT
        self.set_ssh_session_bmc_hostname()
        self.set_common_settings()
        self.set_optional_arguments()
        FwJsonObj = fw_up.FwJson(os.path.dirname(fw_up.__file__))
        self.json = FwJsonObj.get_priority_ordered_json()
        Logger.start(name=self._testMethodName)
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        self.recovery_normal_environment()
        self.dispose_resource()
        pass

    def is_skip_components(self, component):
        if self.skip_components is not None:
            if component in self.skip_components and component in self.json:
                return True
        else:
            return False

    @abstractmethod
    def set_common_settings(self):
        pass

    def set_ssh_session_bmc_hostname(self):
        self.hostname = os.environ.get("TEST_HOSTNAME", self.USERVER_HOSTNAME)
        self.bmc_hostname = os.environ.get("TEST_BMC_HOSTNAME", self.BMC_HOSTNAME)
        if "mpk" in self.bmc_hostname or "fre" in self.bmc_hostname:
            self.bmc_hostname += ".fbinfra.net"
        pass

    def set_optional_arguments(self):
        """
        Parse optional arguments from cit_runner.py --fw-opt-args
        """
        global G_VERBOSE
        optional_args = os.environ.get("TEST_FW_OPT_ARGS", None)
        if optional_args is not None:
            try:
                parser = argparse.ArgumentParser()
                parser.add_argument(
                    "--skip",
                    "-s",
                    help="This is used to skip the components on collective test.",
                )
                parser.add_argument(
                    "--verbose",
                    "-v",
                    help="This is used to enable verbose mode",
                    action="store_true",
                    default=False,
                )
                parser.add_argument(
                    "--force",
                    "-f",
                    help="This is used to force the upgrade",
                    action="store_true",
                    default=False,
                )
                filter_out_chars = ["[", "]", "{", "}", "(", ")"]
                table = str.maketrans(dict.fromkeys("".join(filter_out_chars)))
                optional_args = optional_args.translate(table)
                args = parser.parse_args(optional_args.split())

                if args.force:
                    self.forced_upgrade = True

                if args.verbose:
                    G_VERBOSE = True

                if args.skip:
                    self.skip_components = args.skip.split(",")
            except SystemExit as ex:
                if ex.code == 0:
                    self.skipTest("argument description")
                else:
                    self.skipTest("invalid argument")

    def connect_to_remote_host(self, logging=False):
        """
        method to open ssh session to perform external testing
        """
        if logging:
            self.print_line(
                "Connecting to UUT ", "left", ".", endline=" ", has_append=True
            )
            Logger.info("Connecting to UUT ...")
        try:
            self.bmc_ssh_session = OpenBMCSSHSession(self.bmc_hostname)
            self.bmc_ssh_session.connect()
            self.bmc_ssh_session.login()  # connect to BMC UUT
            self.bmc_ssh_session.session.prompt(timeout=20)  # wait for prompt
            if logging:
                print("Done")
            return True
        except Exception:
            return False

    def is_connection_ready(self):
        """
        Check status of ssh session
        """
        ret = False
        if self.bmc_ssh_session.session.isalive():
            test_cmd = 'echo "CIT TESTING" > /dev/kmsg'
            ret = self.send_command_to_UUT(test_cmd)
        return ret

    def reconnect_to_remote_host(self, timeout=30, logging=False):
        """
        reconnect to remote DUT until the timeout is expired
        """
        if logging:
            self.print_line(
                "Reconnecting to DUT ", "left", ".", endline=" ", has_append=True
            )
        Logger.info("Reconnecting to DUT ...")
        if not self.wait_until(lambda: self.connect_to_remote_host(), timeout):
            print("Failed")
            return False
        time.sleep(self.scm_boot_time)  # waiting for SCM to get ready
        if logging:
            print("Done")
        return True

    def flush_session_contents(self, logging=False):
        try:
            self.bmc_ssh_session.session.prompt()
        except pexpect.exceptions.EOF:
            if logging:
                print("The child has exited!")

    def send_command_to_UUT(self, command, delay=None, prompt=True):
        if delay is None:
            delay = self.command_exec_delay
        self.bmc_ssh_session.session.sendline(command)  # send command
        time.sleep(delay)  # Delay for UUT execute the command
        if prompt:
            return self.bmc_ssh_session.session.prompt(
                timeout=self.command_promtp_timeout
            )  # wait for prompt

    def receive_command_output_from_UUT(self, only_last=False):
        cmd_result = self.bmc_ssh_session.session.before.decode("utf-8")
        if only_last:
            return cmd_result.split("\r\n").pop(-2)
        else:
            return cmd_result

    def power_cycle_UUT_and_close_session(self, logging=False):
        """
        power cycle UUT and close ssh session
        """
        if not self.bmc_ssh_session.session.isalive():
            if not self.reconnect_to_remote_host(30):
                self.fail("Cannot power cycle UUT due to connection lost!")
        time.sleep(10)  # waiting for session to get ready
        if logging:
            self.print_line(
                "Power cycle UUT ", "left", ".", endline=" ", has_append=True
            )
        Logger.info("Power cycle UUT ... ")
        # Power cycle UUT to check the upgrade result
        self.send_command_to_UUT(self.power_reset_cmd, prompt=False)
        time.sleep(10)  # waiting for reboot command to be successful
        self.bmc_ssh_session.session.close()
        time.sleep(5)  # waiting for clearing session
        if logging:
            print("Done")

    def wait_until(self, predicate, timeout: int):
        """
        helper method for waiting some process until it's finished
        or timeout's expired
        """
        end = time.time() + timeout
        while time.time() < end:
            if predicate():
                return True
            else:
                time.sleep(20)
        return False

    def dispose_resource(self):
        """
        dispose method to unset variable and clean up the resource.
        """
        if self.bmc_ssh_session:
            self.bmc_ssh_session.session.close()

    def recovery_normal_environment(self):
        """
        recover UUT to normal environment
        """
        if self.bmc_ssh_session:
            if self.bmc_ssh_session.session.isalive():
                self.send_command_to_UUT(self.START_FSCD)

    def verify_binary_checksum_on_remote_target(self, logging=False):
        """
        verify that the firmware binary hash on UUT matches
        the ones in the json
        """
        if logging:
            self.print_line(
                "Checking binary on UUT ",
                "left",
                ".",
                self.MAX_LINE_LEN - 10,
                endline=" ",
            )
        Logger.info("Checking binary hash on UUT ... ")

        sha1sum_cmd = "sha1sum {} | cut -d ' ' -f 1"
        md5sum_cmd = "md5sum {} | cut -d ' ' -f 1"

        for fw_entity in self.json:

            filename = os.path.join(
                self.remote_bin_path, self.json[fw_entity][UFW_NAME]
            )
            matchingHash = False

            if self.json[fw_entity][UFW_HASH] == HashType.SHA1SUM.value:
                test_cmd = sha1sum_cmd.format(filename)
            elif self.json[fw_entity][UFW_HASH] == HashType.MD5SUM.value:
                test_cmd = md5sum_cmd.format(filename)
            else:
                self.fail("unknow hash type on component {}".format(fw_entity))

            self.send_command_to_UUT(test_cmd)
            binary_hash = self.receive_command_output_from_UUT(only_last=True)
            # verify hash string from UUT
            matchingHash = binary_hash == self.json[fw_entity][UFW_HASH_VALUE]
            self.assertTrue(
                matchingHash,
                "firmware component {} missmatch for file {}".format(
                    fw_entity, filename
                ),
            )
        if logging:
            print("Done")

    def extract_subentity(self):
        """
        extract component subentity to an individual entity
        """
        tmp_json_data = {}
        clone_origin_json = copy.deepcopy(self.json)
        is_sub_entity_exist = False
        for fw_entity in clone_origin_json:
            entity_obj = clone_origin_json.get(fw_entity, None)
            if UFW_ENTITY_INSTANCE in entity_obj:
                is_sub_entity_exist = True
                entityUpgradeObj = FwEntityUpgrader(
                    fw_entity, clone_origin_json, self.upgrader_path
                )
                instance_list = entityUpgradeObj._get_entities_list(
                    entity_obj[UFW_ENTITY_INSTANCE]
                )
                for instance in instance_list:
                    cmd_to_execute = entity_obj.get(UFW_CONDITION, "").format(
                        entity=instance
                    )

                    if (
                        self.check_entity_condition(condition_cmd=cmd_to_execute)
                        or cmd_to_execute == ""
                    ):
                        new_entity_key = "{}_{}".format(fw_entity, instance)
                        new_fw_entity = {
                            new_entity_key: {
                                UFW_GET_VERSION: entity_obj.get(
                                    UFW_GET_VERSION, ""
                                ).format(entity=instance),
                                UFW_CMD: entity_obj.get(UFW_CMD, "").format(
                                    entity=instance, filename="{filename}"
                                ),
                                UFW_PRIORITY: entity_obj.get(UFW_PRIORITY, ""),
                                UFW_VERSION: entity_obj.get(UFW_VERSION, ""),
                                UFW_NAME: entity_obj.get(UFW_NAME, ""),
                                UFW_HASH: entity_obj.get(UFW_HASH, ""),
                                UFW_HASH_VALUE: entity_obj.get(UFW_HASH_VALUE, ""),
                            }
                        }
                        tmp_json_data.update(new_fw_entity)
                        self.upgrading_timeout.update(
                            {new_entity_key: self.upgrading_timeout[fw_entity]}
                        )

            if is_sub_entity_exist:
                self.json.pop(fw_entity)
                is_sub_entity_exist = False

        # some entities contains subentities of various types
        # for example minipack can have both pim16q and pim16o
        # inserted in the same chassis. Since there will always
        # be pim in a units, if a subentities is not supported (aka
        # the pim type is not detected in fpga_ver output) then
        # we should skip that test. Testing for output of fpga_ver
        # is covered in openbmc_cit test to ensure all pims are present
        if not tmp_json_data:
            self.skipTest("This entity type is not supported... skipping this test")

        self.json.update(tmp_json_data)

        orddict = collections.OrderedDict(
            sorted(
                self.json.items(),
                key=lambda k_v: k_v[1][UFW_PRIORITY],
            )
        )
        self.json = orddict

    def check_entity_condition(self, condition_cmd=None) -> bool:
        # return True
        error_list = [condition_cmd, "command not found", "No such file or directory"]
        try:
            self.send_command_to_UUT(condition_cmd)
            is_type_match = self.receive_command_output_from_UUT(only_last=True)
            for error in error_list:
                if error in is_type_match:
                    return False
            return True
        except Exception as e:
            Logger.info("Exception {} occured when running command".format(e))
            return False

    def checking_components_version(self, components=None, logging=False):
        """
        check and compare between firmware package and running version
        on remote host
        """
        if logging:
            self.print_line(
                "Checking version ", "left", ".", endline=" ", has_append=True
            )
        Logger.info("Checking version ...")
        if components is None:
            components = []
        has_upgrading = False
        warning_msg = ""
        warning_list = []

        for fw_entity in self.json:
            need_to_upgrade = False
            check_version_cmd = self.json.get(fw_entity, "").get(UFW_GET_VERSION, "")
            package_ver = self.json.get(fw_entity, "").get(UFW_VERSION, "")

            if len(package_ver) == 0:
                package_ver = "N/A"
                warning_msg = (
                    "{}: Cannot find entity version in JSON, defaulting to upgrade"
                ).format(fw_entity)
                warning_list.append(warning_msg)
                Logger.warn(warning_msg)

            if len(check_version_cmd) == 0:
                current_ver = ""
            else:
                self.send_command_to_UUT(check_version_cmd)
                current_ver = self.receive_command_output_from_UUT(only_last=True)

            version_length = len(current_ver)
            if version_length > 11 or version_length == 0:
                current_ver = "N/A"
                need_to_upgrade = True
                warning_msg = (
                    "{}: Cannot get current version on UUT, defaulting to upgrade"
                ).format(fw_entity)
                warning_list.append(warning_msg)
                Logger.warn(warning_msg)
            else:
                try:
                    entityUpgradeObj = FwEntityUpgrader(
                        fw_entity, self.json, self.upgrader_path
                    )
                    need_to_upgrade = (
                        entityUpgradeObj._compare_current_and_package_versions(
                            current_ver, package_ver
                        )
                    )
                except Exception:
                    pass

            if self.is_skip_components(fw_entity):
                need_to_upgrade = False
            elif self.forced_upgrade:
                need_to_upgrade = True

            if need_to_upgrade:
                has_upgrading = True
            Logger.info("Current version: {}".format(current_ver))
            Logger.info("Package version: {}".format(package_ver))
            components.append(
                {
                    "entity": fw_entity,
                    "current_version": current_ver,
                    "image_version": package_ver,
                    "upgrade_needed": need_to_upgrade,
                    "upgrade_status": True,
                    "execution_time": 0,
                    "result": "",
                }
            )
        if len(components) == 0:
            self.fail("No entity JSON data in json file.")
        if warning_list:
            print("Warning")
            self.print_warning(warning_list)
        else:
            if logging:
                print("Done")
        if not has_upgrading:
            return False
        return True

    def justify_text(self, text, alignment, fillchr, text_width=0):
        if alignment == "center":
            text = text.center(text_width, fillchr)
        elif alignment == "right":
            text = text.rjust(text_width, fillchr)
        elif alignment == "left":
            text = text.ljust(text_width, fillchr)
        else:
            return text
        return text

    def print_line(
        self,
        contents="",
        alignment="left",
        fillchr=" ",
        line_width=None,
        endline="\n",
        has_append=False,
    ):
        """
        print aligned and justified text line
        """
        if line_width is None:
            line_width = self.MAX_LINE_LEN
        if has_append:
            line_width = self.MAX_LINE_LEN - self.MAX_APPEND_LEN
        if isinstance(contents, (list, tuple)):
            word_cnt = len(contents)
            text_width = int(line_width / word_cnt)
            line = ""
            for word in contents:
                line = line + self.justify_text(word, alignment, fillchr, text_width)
            print(line, end=endline)
        else:
            print(
                self.justify_text(contents, alignment, fillchr, line_width), end=endline
            )

    def print_warning(self, messages):
        """
        display warning message while processing.
        """
        if G_VERBOSE:
            print("Warning!")
            if isinstance(messages, list):
                print(*messages, sep="\n")
            else:
                print(messages)

    def show_failed_test_log(self, component_test_data):
        """
        display more details of each failed test case.
        """
        print()
        self.print_line("", "center", "*")
        self.print_line("Failures Summary", "center")
        self.print_line("", "center", "*")
        for component in component_test_data:
            if not component["upgrade_needed"]:
                continue
            entity = component["entity"]
            if not component["upgrade_status"]:
                print("Component: {}".format(entity))
                print("Test log: ")
                self.print_line(" Start ", "center", "=")
                self.print_line(component["result"])
                self.print_line(" End ", "center", "=")

    def summary_test(
        self,
        components_to_upgrade,
        upgraded_components=None,
        verbose=False,
        logging=False,
    ):
        """
        summary test result and show the failed test log
        """
        failed_test = False
        failures_cnt = 0
        if logging:
            print()
            self.print_line("", "center", "*")
            self.print_line("Test Summary", "center")
            self.print_line("", "center", "*")
            self.print_line(
                ["Name", "Previous", "Current", "Package", "   Result", "Time elapsed"],
                "center",
            )
            self.print_line("", "center", "-")

        for index, component in enumerate(components_to_upgrade):
            if not component["upgrade_needed"] and not verbose:
                continue

            if component["upgrade_needed"]:
                if component["upgrade_status"]:
                    result = "   Passed"
                else:
                    result = "   Failed"
                execution_time = "{:.2f}s".format(component["execution_time"])
            else:
                execution_time = "-"
                result = "   Skipped"

            entity = component["entity"]
            if upgraded_components is None:
                current_version = "N/A"
            else:
                current_version = upgraded_components[index]["current_version"]
            if logging:
                self.print_line(
                    [
                        entity,
                        component["current_version"],
                        current_version,
                        component["image_version"],
                        result,
                        execution_time,
                    ],
                    "center",
                )
            if not component["upgrade_status"]:
                failures_cnt += 1
                failed_test = True

        if failed_test:
            self.show_failed_test_log(components_to_upgrade)
            self.fail("{} components failed".format(failures_cnt))
        if logging:
            self.print_line("", "center", "-")

    def upgrade_components(self, components_to_upgrade, logging=False):
        """
        main method to check and upgrade the components
        """
        for component in components_to_upgrade:
            # Start timestamp
            start = time.perf_counter()
            entity = component["entity"]
            ret = -1
            if component["upgrade_needed"]:
                try:
                    if not self.bmc_ssh_session.session.isalive():
                        print("remote ssh session broke! retrying...")
                        if not self.reconnect_to_remote_host(30):
                            self.fail("cannot reconnect to UUT!")
                    filename = self.remote_bin_path + "/" + self.json[entity][UFW_NAME]
                    Logger.info("ander-updating: filename is {}".format(filename))
                    cmd_to_execute = self.json[entity][UFW_CMD]
                    cmd_to_execute = cmd_to_execute.format(filename=filename)
                    if logging:
                        self.print_line(
                            "Updating: {} ".format(entity),
                            "left",
                            ".",
                            endline=" ",
                            has_append=True,
                        )
                    self.send_command_to_UUT(self.STOP_FSCD)
                    self.send_command_to_UUT(self.STOP_WDT)
                    self.send_command_to_UUT(cmd_to_execute, prompt=False)
                    ret = self.bmc_ssh_session.session.expect_exact(
                        self.expected_keyword, timeout=self.upgrading_timeout[entity]
                    )
                except pexpect.exceptions.TIMEOUT:
                    ret = -1
                    if logging:
                        print("Timed out")
                except Exception as e:
                    self.fail(e)

                # End timestamp
                end = time.perf_counter()
                component["execution_time"] = end - start
                if ret >= 0 and ret <= self.num_last_failed_expected_key:
                    if logging:
                        print("Failed")
                    component["upgrade_status"] = False
                    component["result"] = self.receive_command_output_from_UUT()
                    Logger.error("{}: Upgrading failed!".format(component))
                else:
                    if logging and ret == -1:
                        print("Done")
            # flush the rest log.
            self.flush_session_contents(logging)
            if logging:
                print("Done")
            time.sleep(10)  # delay for the current process to be done

    def do_external_firmware_upgrade(self, component=None):
        """
        Main function to perform collective and individual firmware upgrade
        for components according to entity in json file
        To perform individual firmware upgrading, please pass the "component"
        parameter is the name of component. It's supposed to match the entity
        name in json file. e.g. bios, scm, ... etc.
        """
        Logger.info("Start firmware upgrade test!")
        components_to_upgrade = []
        upgraded_components = []
        json_data = {}

        # Set up json data for individual test
        if component is not None:
            json_data[component] = self.json.get(component, "")
            if json_data[component] == "":
                self.skipTest('Component "{}" does not exist'.format(component))
            self.json = json_data

        if G_VERBOSE:
            print("Start firmware upgrade test!")

        # Connect to UUT
        self.connect_to_remote_host(logging=G_VERBOSE)

        # Test connection
        if not self.is_connection_ready():
            if not self.reconnect_to_remote_host(
                self.bmc_reconnect_timeout, logging=G_VERBOSE
            ):
                self.fail("Cannot establish the connection to UUT!")

        self.extract_subentity()

        # Check version and prepare upgrade list
        if not self.checking_components_version(
            components_to_upgrade, logging=G_VERBOSE
        ):
            # No need upgrading for all components
            # Summary the test with verbose flag
            self.summary_test(components_to_upgrade, verbose=True, logging=G_VERBOSE)
            return

        # Verify binaries checksom on UUT
        self.verify_binary_checksum_on_remote_target(logging=G_VERBOSE)

        # Main upgrading process
        self.upgrade_components(components_to_upgrade, logging=G_VERBOSE)

        # Send power cycle command to UUT and clear old ssh session
        self.power_cycle_UUT_and_close_session(logging=G_VERBOSE)

        # Wait and try to reconnect once the UUT is come back
        if not self.reconnect_to_remote_host(
            self.bmc_reconnect_timeout, logging=G_VERBOSE
        ):
            # Print test result and fail the test.
            self.summary_test(components_to_upgrade, verbose=True, logging=True)
            self.fail(
                "Cannot get the running firmware version, UUT is no longer accesible"
            )

        # Get the current version of components on UUT
        self.checking_components_version(upgraded_components, logging=G_VERBOSE)

        # Summary test result for only collective test
        self.summary_test(
            components_to_upgrade, upgraded_components, G_VERBOSE, logging=G_VERBOSE
        )
