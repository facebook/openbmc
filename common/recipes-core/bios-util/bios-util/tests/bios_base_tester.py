# Copyright 2004-present Facebook. All Rights Reserved.

import inspect
import logging
import os
import sys
import time
import traceback
import unittest
from contextlib import contextmanager
from StringIO import StringIO

from bios_board import check_bios_util


# Inorder for unittests to know the location of bios-util here is way for it.
# Ideally if yocto has a framework to integrate these unit tests this wouldnt
# be needed.
cmd_folder = (
    os.path.realpath(
        os.path.abspath(os.path.split(inspect.getfile(inspect.currentframe()))[0])
    )
).rstrip("_test")
if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)


TEST_BIOS_UTIL_CONFIG = "./test-data/config-bios-example-support-test.json"
TEST_BIOS_UTIL_DEFAULT_CONFIG = (
    "./test-data/config-bios-example-default-support-test.json"
)


@contextmanager
def captured_output():
    new_out, nwe_err = StringIO(), StringIO()
    old_out, old_err = sys.stdout, sys.stderr
    try:
        sys.stdout, sys.stderr = new_out, nwe_err
        yield sys.stdout, sys.stderr
    finally:
        sys.stdout, sys.stderr = old_out, old_err


class BaseBiosUtilUnitTest(unittest.TestCase):
    def setUp(self):
        # Defines instructions that will be executed before each test
        # method.
        # Unfortunately in python 2.6.28 setUp and tearDown get called
        # for each test. in python 2.7 and 3 there are different methods at
        # class level that will avoid running them for each test.
        self.define_biosutil()

    def tearDown(self):
        # Defines instructions that will be executed after each test
        # method.
        pass

    def define_biosutil(
        self,
        config=TEST_BIOS_UTIL_CONFIG,
        default_config=TEST_BIOS_UTIL_DEFAULT_CONFIG,
        argv=sys.argv,
    ):
        # Give Get boot_order command to get bios-util option configs
        argv = ["", "FRU", "--boot_order", "get"]
        with captured_output() as (out, err):
            self.bios_tester = check_bios_util(config, default_config, argv)


class Result(unittest.TestResult):
    def startTest(self, test):
        self.startTime = time.time()
        super(Result, self).startTest(test)

    def addSuccess(self, test):
        elapsed = time.time() - self.startTime
        super(Result, self).addSuccess(test)
        print(("\033[32mPASS\033[0m %s (%.3fs)" % (test.id(), elapsed)))

    def addSkip(self, test, reason):
        elapsed = time.time() - self.startTime
        super(Result, self).addSkip(test, reason)
        print(("\033[33mSKIP\033[0m %s (%.3fs) %s" % (test.id(), elapsed, reason)))

    def addExpectedFailure(self, test, err):
        elapsed = time.time() - self.startTime
        super(Result, self).addExpectedFailure(test, err)
        print(
            (
                "\033[33mSKIP(ExpectedFailure)\033[0m %s (%.3fs) %s"
                % (test.id(), elapsed, err)
            )
        )

    def __printFail(self, test, err):
        elapsed = time.time() - self.startTime
        t, val, trace = err
        print(
            (
                "\033[31mFAIL\033[0m %s (%.3fs)\n%s"
                % (
                    test.id(),
                    elapsed,
                    "".join(traceback.format_exception(t, val, trace)),
                )
            )
        )

    def addFailure(self, test, err):
        self.__printFail(test, err)
        super(Result, self).addFailure(test, err)

    def addError(self, test, err):
        self.__printFail(test, err)
        super(Result, self).addError(test, err)
