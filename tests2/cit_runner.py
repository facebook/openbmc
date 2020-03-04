#!/usr/bin/env python
import argparse
import os
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
    }

    def __init__(self, platform, start_dir=BMC_START_DIR, pattern="test*.py"):
        self.platform = platform
        self.tests_set = []
        self.formatted_tests_set = []
        self.start_dir = start_dir + args.platform + "/"
        self.pattern = pattern

    def discover_tests(self):
        suite = unittest.defaultTestLoader.discover(self.start_dir, self.pattern)
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


def get_tests(platform, start_dir, pattern=None):
    if pattern:
        return Tests(platform, start_dir, pattern).get_all_platform_tests()
    return Tests(platform, start_dir).get_all_platform_tests()


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
    Running stress tests on target BMC: test pattern "stress_*"

    Usage Examples:
    On devserver:
    List tests : python cit_runner.py --platform wedge100 --list-tests --start-dir tests/
    List tests that need to connect to BMC: python cit_runner.py --platform wedge100 --list-tests --start-dir tests/ --external --host "NAME"
    Run tests that need to connect to BMC: python cit_runner.py --platform wedge100 --start-dir tests/ --external --host "NAME"
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
            "galaxy100",
            "cmm",
            "minipack",
            "fbtp",
            "fby2",
            "yosemite",
            "lightning",
            "fbttn",
            "yamp",
            "elbert",
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
    pattern = None

    if args.external:
        pattern = "external*.py"
        set_external(args)

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

    test_paths = get_tests(args.platform, args.start_dir, pattern=pattern)
    if args.list_tests:
        for item in test_paths:
            print(item)
        clean_on_exit(0)

    if args.repeat:
        repeat_test(test_paths, args.repeat)
    else:
        RunTest().run_multiple_tests(test_paths)
    clean_on_exit(0)
