# Copyright 2015-present Facebook. All Rights Reserved.
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
import abc
from fsc_util import Logger
from subprocess import Popen, PIPE


class FscSensorBase(object):
    '''
    Fsc sensor base class
    '''

    def __init__(self, **kwargs):
        if 'name' in kwargs:
            self.name = kwargs['name']
        if 'read_source' in kwargs:
            self.read_source = kwargs['read_source']
        if 'write_source' in kwargs:
            self.write_source = kwargs['write_source']

    @abc.abstractmethod
    def read(self, **kwargs):
        '''
        Read value from read_source
        '''
        return

    def write(self, **kwargs):
        '''
        Write value to write_source
        '''
        pass


class FscSensorSourceSysfs(FscSensorBase):
    '''
    Class for FSC sensor source for sysfs
    '''
    def read(self, **kwargs):
        '''
        Reads all sensors values from sysfs source and return data read.
        There are two kinds of sensors temperature and fans.

        Arguments:
            kwargs: set of aruments needed to read from sysfs

        Return:
            blob of data read from sysfs
        '''
        Logger.warn("TODO: Implement read for sysfs")

    def write(self, value):
        '''
        Writes to write_source using echo to sysfs location
        echo #value > sysfs_path

        Arguments:
            value: value to be set to the sensor

        Return:
            N/A
        '''
        Logger.warn("TODO: Implement write for sysfs")


class FscSensorSourceUtil(FscSensorBase):
    '''
    Class for FSC sensor source for util
    '''

    def read(self, **kwargs):
        '''
        Reads all sensors values from the util and return data read.
        There are two kinds of sensors temperature and fans. Following
        are the util usages:
        sensor util: 'util <fru name>' Reads all sensors from a specific fru
        fan util: 'util' Reads all fan speeds

        Arguments:
            kwargs: set of aruments needed to read from any of the util

        Return:
            blob of data read from util
        '''
        cmd = self.read_source
        if 'fru' in kwargs:
            cmd = cmd + " " + kwargs['fru']
        Logger.debug("Reading data with cmd=%s" % cmd)
        try:
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        except:
            Logger.crit("Exception with cmd=%s response=%s" % (cmd, data))
        return data

    def write(self, value):
        '''
        Writes to write_source using util.

        Arguments:
            value: value to be set to the sensor

        Return:
            N/A
        '''
        cmd = self.write_source + " " + str(value) + " " + self.name
        Logger.debug("Setting value using cmd=%s" % cmd)
        try:
            response = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
            if response.find("Setting") == -1:
                raise Exception("Write failed with response=%s" % response)
        except:
            Logger.crit("Exception with cmd=%s response=%s" % (cmd, response))
