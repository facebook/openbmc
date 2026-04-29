#
# SPDX-License-Identifier: MIT
#

from subprocess import Popen, PIPE

from oeqa.runtime.case import OERuntimeTestCase
from oeqa.core.decorator.oetimeout import OETimeout

class ZephyrTest(OERuntimeTestCase):

    @OETimeout(120)
    def test_boot_zephyr(self):

        success = False
        logfile = self.target.qemurunnerlog

        while True:
            line = self.target.serial_readline().decode("utf-8")

            # All good
            if "PROJECT EXECUTION SUCCESSFUL" in line:
                success = True
                break

            if "PROJECT EXECUTION FAILED" in line:
                success = False
                self.assertTrue(success, msg='PROJECT EXECUTION FAILED in file:///%s' % logfile)
                break

            # Most likely cause for faults is incorrectly compiled code
            if "***** USAGE FAULT *****" in line:
                success = False
                self.assertTrue(success, msg='***** USAGE FAULT *****" in file:///%s' % logfile)
                break

        # test program finished, complain if no success message
        self.assertTrue(success, msg='"PROJECT EXECUTION SUCCESSFUL" not in file:///%s' % logfile)
