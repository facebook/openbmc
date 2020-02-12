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
import os
import re
from subprocess import PIPE, Popen

from fsc_util import Logger


class FscSensorBase(object):
    """
    Fsc sensor base class
    """

    def __init__(self, **kwargs):
        if "name" in kwargs:
            self.name = kwargs["name"]
        if "inf" in kwargs:
            self.inf = kwargs["inf"]
        if "read_source" in kwargs:
            self.read_source = kwargs["read_source"]
        if "write_source" in kwargs:
            self.write_source = kwargs["write_source"]
            if "max_duty_register" in kwargs:
                self.max_duty_register = kwargs["max_duty_register"]
            else:
                self.max_duty_register = 100
        else:
            self.write_source = None

        self.read_source_fail_counter = 0
        self.write_source_fail_counter = 0
        self.read_source_wrong_counter = 0
        self.hwmon_source = None
        self.last_error_time = 0
        self.last_error_level = None

    @abc.abstractmethod
    def read(self, **kwargs):
        """
        Read value from read_source
        """
        return

    def write(self, **kwargs):
        """
        Write value to write_source
        """
        pass


class FscSensorSourceSysfs(FscSensorBase):
    """
    Class for FSC sensor source for sysfs
    """

    def get_hwmon_source(self):
        # After BMC comes up this hwmon device is setup once and we can cache
        # that data for the first time instead of determining the source each
        # time
        if self.hwmon_source is not None:
            return self.hwmon_source

        self.hwmon_source = None
        result = re.split("hwmon", self.read_source)
        if os.path.isdir(result[0]):
            construct_hwmon_path = result[0] + "hwmon"
            x = None
            for x in os.listdir(construct_hwmon_path):
                if x.startswith("hwmon"):
                    construct_hwmon_path = (
                        construct_hwmon_path + "/" + x + "/" + result[2].split("/")[1]
                    )
                    if os.path.exists(construct_hwmon_path):
                        self.hwmon_source = construct_hwmon_path
                        return self.hwmon_source

    def read(self, **kwargs):
        """
        Reads all sensors values from sysfs source and return data read.
        There are two kinds of sensors temperature and fans.

        Arguments:
            kwargs: set of aruments needed to read from sysfs

        Return:
            blob of data read from sysfs
        """

        # IF read_source has hwmon* then determine what is the hwmon device
        # and use that for reading
        readsysfs = self.read_source
        if "hwmon*" in self.read_source:
            readsysfs = self.get_hwmon_source()

        cmd = "cat " + readsysfs
        Logger.debug("Reading data with cmd=%s" % cmd)
        data = ""
        try:
            proc = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
            data = proc.stdout.read().decode()
            err = proc.stderr.read().decode()
            if err:
                self.read_source_fail_counter += 1
            else:
                self.read_source_fail_counter = 0
        except SystemExit:
            Logger.debug("SystemExit from sensor read")
            self.read_source_fail_counter += 1
            raise
        except Exception:
            Logger.crit("Exception with cmd=%s response=%s" % (cmd, data))
            self.read_source_fail_counter += 1
        return data

    def write(self, value):
        """
        Writes to write_source using echo to sysfs location
        echo #value > sysfs_path

        Arguments:
            value: value to be set to the sensor

        Return:
            N/A
        """
        if self.write_source is None:
            return
        cmd = (
            "echo "
            + str(value * self.max_duty_register / 100)
            + " > "
            + self.write_source
        )
        Logger.debug("Setting value using cmd=%s" % cmd)
        response = ""
        try:
            response = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        except SystemExit:
            Logger.debug("SystemExit from sensor write")
            raise
        except Exception:
            Logger.crit("Exception with cmd=%s response=%s" % (cmd, response))


class FscSensorSourceUtil(FscSensorBase):
    """
    Class for FSC sensor source for util
    """

    def read(self, **kwargs):
        """
        Reads all sensors values from the util and return data read.
        There are two kinds of sensors temperature and fans. Following
        are the util usages:
        sensor util: 'util <fru name>' Reads all sensors from a specific fru
                     'util <fru name> <sensor number>' Reads sensor from a specific fru number
        fan util: 'util' Reads all fan speeds

        Arguments:
            kwargs: set of aruments needed to read from any of the util

        Return:
            blob of data read from util
        """
        cmd = self.read_source
        if "fru" in kwargs:
            if "inf" in kwargs and kwargs["inf"] is not None:
                cmd += " " + kwargs["fru"] +" --filter"
                inf =  kwargs["inf"]
                for name in inf["ext_vars"]:
                    sdata = name.split(":")
                    board = sdata[0]
                    if board != kwargs["fru"]:
                        continue
                    #sname = sdata[1]
                    cmd += " " + sdata[1]
            elif "num" in kwargs and len(kwargs["num"]):
                cmd = ""
                for num in kwargs["num"]:
                    cmd += self.read_source + " " + kwargs["fru"] + " " + num + ";"
            else:
                cmd = cmd + " " + kwargs["fru"]
        Logger.debug("Reading data with cmd=%s" % cmd)
        data = ""
        try:
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        except SystemExit:
            Logger.debug("SystemExit from sensor read")
            raise
        except Exception:
            Logger.crit("Exception with cmd=%s response=%s" % (cmd, data))
        return data

    def write(self, value):
        """
        Writes to write_source using util.

        Arguments:
            value: value to be set to the sensor

        Return:
            N/A
        """
        if self.write_source is None:
            return
        cmd = self.write_source % (int(value * self.max_duty_register / 100))
        Logger.debug("Setting value using cmd=%s" % cmd)
        response = ""
        try:
            response = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
            if response.find("Error") != -1:
                raise Exception("Write failed with response=%s" % response)
        except SystemExit:
            Logger.debug("SystemExit from sensor write")
            raise
        except Exception:
            Logger.crit("Exception with cmd=%s response=%s" % (cmd, response))
