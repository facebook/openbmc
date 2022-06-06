#!/usr/bin/env python3
import argparse
import os
import sys
import unittest

BMC_START_DIR = "/usr/local/bin/tests2/tests/"


class RunTest:
    def __init__(self):
        self.testrunner = unittest.TextTestRunner(verbosity=2)
        self.testloader = unittest.defaultTestLoader

    def get_single_test(self, test_path):
        return self.testloader.loadTestsFromName(test_path)

    def run_single_test(self, test_path):
        """
        test_path example - tests.wedge100.test_eeprom.EepromTest.test_odm_pcb
        """
        return self.testrunner.run(self.get_single_test(test_path))

    def get_multiple_tests(self, test_paths):
        return self.testloader.loadTestsFromNames(test_paths)

    def run_multiple_tests(self, test_paths):
        return self.testrunner.run(self.get_multiple_tests(test_paths))


class Tests:

    # For python unittest the way discovery works is that common imported tests
    # are imported but we dont run them and rely on platform to drive tests.
    # By default all tests in base classes cannot be run , hence "common"
    IGNORE_TEST_PATTERN_MAP = {
        "default": ["common"],
        "yamp": [
            "common",
            "minipack",
        ],  # So yamp can import minipack tests and run them only one time
        "elbert": [
            "common",
            "minipack",
        ],  # So elbert can import minipack tests and run them only one time
        "fbdarwin:": [
            "common",
            "minipack",
        ], # So fbdarwin can import minipack tests and run them only one time
    }

    def __init__(self, platform, start_dir, pattern, denylist):
        self.platform = platform
        self.tests_set = []
        self.formatted_tests_set = []
        self.start_dir = start_dir + args.platform + "/"
        self.pattern = pattern
        self.denylist = denylist

    def discover_tests(self):
        loader = unittest.defaultTestLoader
        suite = loader.discover(self.start_dir, self.pattern)
        if loader.errors:
            for _error in loader.errors:
                print(_error)
            sys.exit(1)
        return suite

    def filter_based_on_pattern(self, test_string):
        if self.platform in self.IGNORE_TEST_PATTERN_MAP.keys():
            ignore_list = self.IGNORE_TEST_PATTERN_MAP[self.platform]
        else:
            ignore_list = self.IGNORE_TEST_PATTERN_MAP["default"]

        for item in ignore_list:
            if item in test_string:
                return ""
        return test_string

    def get_tests(self, suite):
        if hasattr(suite, "__iter__"):
            for item in suite:
                self.get_tests(item)
        else:
            self.tests_set.append(self.filter_based_on_pattern(str(suite)))
        return self.tests_set

    def format_into_test_path(self, testitem):
        """
        Given string like "test_odm_pcb (tests.wedge100.test_eeprom.EepromTest)"
        convert to a python test format to use it directly
        return value example : tests.wedge100.test_eeprom.EepromTest.test_odm_pcb
        """
        test_string = testitem.split("(")
        test_path = test_string[1].split(")")[0] + "." + test_string[0]
        return test_path.strip()

    def get_all_platform_tests(self):
        """
        Returns a set of test names that are formatted to a single test like
        'tests.wedge100.test_eeprom.EepromTest.test_odm_pcb'
        """
        for testitem in self.get_tests(self.discover_tests()):
            if not testitem:
                continue
            prefix = "tests." + self.platform + "."
            self.formatted_tests_set.append(
                prefix + self.format_into_test_path(testitem)
            )

        if self.denylist:
            try:
                with open(self.denylist, "r") as f:
                    denylist = f.read().splitlines()
            except FileNotFoundError:
                denylist = []

            self.formatted_tests_set = [
                t for t in self.formatted_tests_set if t not in denylist
            ]

        return self.formatted_tests_set


def set_external(args):
    """
    Tests that trigger from outside BMC need Hostname information to BMC and userver
    """
    if args.host:
        os.environ["TEST_HOSTNAME"] = args.host
        # If user gave a hostname, determine oob name from it and set it.
        if "." in args.host:
            index = args.host.index(".")
            os.environ["TEST_BMC_HOSTNAME"] = (
                args.host[:index] + "-oob" + args.host[index:]
            )

    if args.bmc_host:
        os.environ["TEST_BMC_HOSTNAME"] = args.bmc_host


def set_fw_args(args):
    """
    Optional arguments for firmware upgrade test
    """
    os.environ["TEST_FW_OPT_ARGS"] = args.firmware_opt_args


def clean_on_exit(returncode):
    # Resetting hostname
    os.environ["TEST_HOSTNAME"] = ""
    os.environ["TEST_BMC_HOSTNAME"] = ""
    exit(returncode)


def arg_parser():
    cit_description = """
    CIT supports running following classes of tests:

    Running tests on target BMC: test pattern "test_*"
    Running tests on target BMC & CPU from outside BMC: test pattern "external_*"
    Running tests on target BMC & CPU from outside BMC: test pattern "external_fw_upgrade*"
    Running stress tests on target BMC: test pattern "stress_*"

    Usage Examples:
    On devserver:
    List tests : python cit_runner.py --platform wedge100 --list-tests --start-dir tests/
    List tests that need to connect to BMC: python cit_runner.py --platform wedge100 --list-tests --start-dir tests/ --external --host "NAME"
    List real upgrade firmware external tests that connect to BMC: python cit_runner.py --platform wedge100 --list-tests --start-dir tests/ --upgrade-fw
    Run tests that need to connect to BMC: python cit_runner.py --platform wedge100 --start-dir tests/ --external --bmc-host "NAME"
    Run real upgrade firmware external tests that connect to BMC: python cit_runner.py --platform wedge100 --run-tests "path" --upgrade --bmc-host "NAME" --firmware-opt-args="-f -v"
    Run single/test that need connect to BMC: python cit_runner.py --run-test "path" --external --host "NAME"

    On BMC:
    List tests : python cit_runner.py --platform wedge100 --list-tests
    Run tests : python cit_runner.py --platform wedge100
    Run single test/module : python cit_runner.py --run-test "path"
    """

    parser = argparse.ArgumentParser(
        epilog=cit_description, formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        "--platform",
        "-p",
        help="Run all tests in platform by platform name",
        choices=[
            "wedge",
            "wedge100",
            "wedge400",
            "wedge400c",
            "cloudripper",
            "galaxy100",
            "cmm",
            "minipack",
            "fbtp",
            "fby2",
            "northdome",
            "fuji",
            "yosemite",
            "lightning",
            "fbttn",
            "yamp",
            "elbert",
            "fbdarwin",
            "fby3",
            "grandcanyon",
            "fby35",
            "churchillmono",
        ],
    )
    parser.add_argument(
        "--run-test",
        "-r",
        help="Path to run a single test. Example: \
                        tests.wedge100.test_eeprom.EepromTest.test_odm_pcb",
    )

    parser.add_argument(
        "--list-tests", "-l", action="store_true", help="List all available tests"
    )

    parser.add_argument(
        "--cli-logging",
        "-c",
        action="store_true",
        help="To log all cli being called during the cit run, the output will store at /tmp/cli.txt",
    )

    parser.add_argument(
        "--start-dir",
        "-s",
        help="Path for where test discovery should start \
                        default: /usr/local/bin/tests2/tests/",
        default=BMC_START_DIR,
    )

    parser.add_argument("--stress", help="run all stress tests", action="store_true")
    parser.add_argument(
        "--external",
        help="Run tests from outside BMC, these are tests that have \
                        pattern external_test*.py, require --host to be set",
        action="store_true",
        default=False,
    )  # find better way to represent this ?

    parser.add_argument(
        "--upgrade-fw",
        help="Run tests from outside BMC, these are tests that have \
                        pattern external_test*.py, require --host to be set",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "--firmware-opt-args",
        help="Set optional arguments for external firmware upgrading \
                        -To skip upgrading of desired components, please use the \
                        component name e.g bios, scm ... etc with '--skip'. \
                        -To show summary skipped components information, and \
                        enable verbose mode , add '--verbose' to the argument string. \
                        -To force to upgrade all components(except skipped comps) \
                        , add '--force' to the argument string.\
                        example: --firmware-opt-args='--skip=bios,scm --verbose --force'",
        type=str,
    )

    parser.add_argument(
        "--fw-upgrade",
        help="Firmware upgrade test, these are tests that have \
                        pattern fw_test*.py, require --host to be set",
        action="store_true",
        default=False,
    )

    parser.add_argument(
        "--host",
        help="Used for running tests\
                        external to BMC and interacting with BMC and main CPU (ONLY) \
                        example: hostname/ip of main CPU",
    )

    parser.add_argument(
        "--bmc-host",
        help="Used for running tests\
                        external to BMC and interacting with BMC and main CPU (ONLY) \
                        example: hostname/ip of BMC",
    )

    parser.add_argument("--repeat", help="Used to repeat tests many times")
    parser.add_argument("--denylist", help="File specifying tests to ignored")

    return parser.parse_args()


def repeat_test(test, num_iter, single=False):
    fail_counter = 0
    fails = []
    for i in range(int(num_iter)):
        print("\nTest Iteration #{}".format(i + 1))
        if single:
            test_result = RunTest().run_single_test(test)
        else:
            test_result = RunTest().run_multiple_tests(test)
        if not test_result.wasSuccessful():
            fail_counter = fail_counter + 1
            fails.append(i + 1)
    if fail_counter > 0:
        print("Test failed {} times".format(fail_counter))
        print("Test failed at these iterations: {}".format(fails))
        clean_on_exit(1)
    else:
        clean_on_exit(0)


if __name__ == "__main__":
    args = arg_parser()
    pattern = "test*.py"

    if args.platform == "wedge400c":
        args.platform = "wedge400"

    if args.upgrade_fw:
        pattern = "external_fw_upgrade*.py"
        set_external(args)

    if args.external:
        pattern = "external_test*.py"
        set_external(args)

    if args.firmware_opt_args:
        set_fw_args(args)

    if args.fw_upgrade:
        pattern = "fw*.py"

    if args.stress:
        pattern = "stress*.py"

    if args.run_test:
        if args.repeat:
            repeat_test(args.run_test, args.repeat, single=True)
        else:
            test_result = RunTest().run_single_test(args.run_test)
            rc = 0 if test_result.wasSuccessful() else 1
            clean_on_exit(rc)

    if not args.platform:
        print("Platform needed to run tests, pass --platform arg. Exiting..")
        clean_on_exit(1)

    tests = Tests(args.platform, args.start_dir, pattern, args.denylist)
    test_paths = tests.get_all_platform_tests()
    if args.list_tests:
        for item in test_paths:
            print(item)
        clean_on_exit(0)

    if args.repeat:
        repeat_test(test_paths, args.repeat)
    else:
        RunTest().run_multiple_tests(test_paths)
    clean_on_exit(0)
