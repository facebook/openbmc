#!/usr/bin/env python
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

import time
import traceback
import unittest


def print_result(testResult):
    print(("\x1b[6;30;32m" + "Total Tests=" + str(testResult.testsRun)))
    print(("\x1b[6;30;31m" + "Total Failed Tests=" + str(len(testResult.failures))))
    print(("\x1b[6;30;31m" + "Total Errored Tests=" + str(len(testResult.errors))))

    if len(testResult.failures) > 0:
        print(("\x1b[6;30;31m" + "\nDetailed Failed Tests="))
        fail_dct = dict(testResult.failures)
        for key, value in list(fail_dct.items()):
            print(("\x1b[6;30;31m" + "\n " + str(key) + str(value)))

    if len(testResult.errors) > 0:
        print(("\x1b[6;30;31m" + "\nDetailed Errored Tests="))
        errors_dct = dict(testResult.errors)
        for key, value in list(errors_dct.items()):
            print(("\x1b[6;30;31m" + "\n " + str(key) + str(value)))

    print("\x1b[0m")


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
