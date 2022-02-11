#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
import os
import sys

from utils.cit_logger import Logger


# When tests are discovered upgrader is not yet installed,
# catch the import failure
try:
    # Upgrader and binaries need to be installed in /tmp
    sys.path.append("/tmp/fw_upgrade")
    import fw_json as fw_up
    from entity_upgrader import FwUpgrader
    from constants import UFW_NAME, UFW_HASH, UFW_HASH_VALUE, HashType, UFW_PRIORITY
except Exception:
    pass


class BaseUpgradeAllDryRunTest(object):
    def setUp(self):
        self.binPath = os.path.dirname(fw_up.__file__)
        FwJsonObj = fw_up.FwJson(self.binPath)
        self.json = FwJsonObj.get_priority_ordered_json()
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def set_binary_paths(self):
        pass

    def test_fw_upgrade_dryrun(self):
        """
            Test that the firmware binary hash matches the ones in the json
        """
        # If everything is good this test passes. If anything went wrong, we should see an
        # exception. Seeing an exception is a test failure, python unittest does have
        # "expect a no exception" support.
        FwUpgrader(self.json, self.binPath, dryrun=True).run_upgrade()
