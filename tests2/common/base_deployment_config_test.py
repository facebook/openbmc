# Copyright 2022-present Facebook. All Rights Reserved.
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


from abc import abstractmethod

from utils.cit_logger import Logger


class DeploymentConfig(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def get_device_config(self):
        pass

    @abstractmethod
    def get_desired_device_config(self):
        pass

    def test_deployment_config(self):
        Logger.info("Testing the deploymnet config")

        desired_configs = self.get_desired_device_configs()
        current_config = self.get_device_config()

        if current_config not in desired_configs:
            Logger.error(current_config)
            Logger.error("Expected config should match one of the below config")
            i = 1
            for config in desired_configs:
                Logger.error("-------------------")
                Logger.error("Config - %d " % (i))
                Logger.error("-------------------")
                i = i + 1
                Logger.error(config)

            self.fail(
                "Current Device configuration is not matching Expected Configuration"
            )
