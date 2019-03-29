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
import unittest
import re
from abc import abstractmethod
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd

class BaseSensorsTest(unittest.TestCase):

    def setUp(self):
        '''
        For now the scope if for sensors exposed from lm sensors
        '''
        self.start_logging(__name__)
        self.sensors_cmd = None
        self.parsed_result = None

    def tearDown(self):
        pass

    def start_logging(self, name):
        Logger.start(name)

    @abstractmethod
    def set_sensors_cmd(self):
        pass

    @abstractmethod
    def parse_sensors(self):
        pass


class LmSensorsTest(BaseSensorsTest):

    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors"]

    def get_parsed_result(self):
        if not self.parsed_result:
            self.parsed_result = self.parse_sensors()
        return self.parsed_result

    def parse_sensors(self):
        self.set_sensors_cmd()
        data = run_shell_cmd(self.sensors_cmd)
        result = {}

        data = re.sub('\(.+?\)', '', data)
        for edata in data.split('\n\n'):
            adata = edata.split('\n', 1)
            if (len(adata) < 2):
                break;
            for sdata in adata[1].split('\n'):
                tdata = sdata.split(':')
                if (len(tdata) < 2):
                    continue
                if 'Adapter' in tdata:
                    continue
                result[tdata[0].strip()] = tdata[1].strip()
        return result
