# Copyright 2004-present Facebook. All Rights Reserved.

import inspect
import logging
import os
import sys
import time
import traceback
import unittest

from fscd import Fscd


# Inorder for unittests to know the location of fscd here is way for it.
# Ideally if yocto has a framework to integrate these unit tests this wouldnt
# be needed.
cmd_folder = (
    os.path.realpath(
        os.path.abspath(os.path.split(inspect.getfile(inspect.currentframe()))[0])
    )
).rstrip("_test")
if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)


TEST_CONFIG = "./test-data/config-example-test.json"
TEST_ZONE = "./test-data/"


class BaseFscdUnitTest(unittest.TestCase):
    def setUp(self):
        """
        Defines instructions that will be executed before each test
        method.
        """
        # Unfortunately in python 2.6.28 setUp and tearDown get called
        # for each test. in python 2.7 and 3 there are different methods at
        # class level that will avoid running them for each test.
        self.define_fscd()
        self.define_logger()
        # Comment out this log if you want to see more logs from tests
        logging.disable(logging.CRITICAL)

    def tearDown(self):
        """
        Defines instructions that will be executed after each test
        method.
        """
        pass

    def define_fscd(self, config=TEST_CONFIG, zone_config=TEST_ZONE):
        self.fscd_tester = Fscd(config, zone_config)
        self.fscd_tester.builder()

    def define_logger(self):
        logger = logging.getLogger()
        logger.level = logging.DEBUG
        stream_handler = logging.StreamHandler(sys.stdout)
        logger.addHandler(stream_handler)


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
