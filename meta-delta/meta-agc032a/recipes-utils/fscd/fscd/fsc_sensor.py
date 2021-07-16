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
from kv import kv_get, kv_set
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
        if "write_type" in kwargs:
            self.write_type = kwargs["write_type"]
        else:
            self.write_type = None

        if "filter" in kwargs:
            self.filter = kwargs["filter"]
        else:
            self.filter = None

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

        try:
            with open(readsysfs) as f:
                return f.read()

        except Exception as e:
            Logger.crit(
                "Exception while trying to read {readsysfs}: {e}".format(
                    readsysfs=repr(readsysfs), e=repr(e)
                )
            )
            self.read_source_fail_counter += 1
            raise

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

        svalue = str(value * self.max_duty_register // 100)

        try:
            with open(self.write_source, "w") as f:
                f.write(svalue)

        except Exception as e:
            Logger.crit(
                "Exception while trying to write {write_source} with sensor value {svalue} : {e}".format(  # noqa: B950
                    write_source=repr(self.write_source), svalue=repr(svalue), e=repr(e)
                )
            )
            raise


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


class FscSensorSourceKv(FscSensorBase):
    """
    Class for FSC sensor source for kv
    """

    def read(self, **kwargs):
        """
        Reads all sensors values from kv source and return data read.
        There are two kinds of sensors temperature and fans.

        Arguments:
            kwargs: set of aruments needed to read from kv

        Return:
            blob of data read from kv
        """

        Logger.debug("Reading data with kv=%s" % self.read_source)
        data = ""
        try:
            data = kv_get(self.read_source)
        except SystemExit:
            Logger.debug("SystemExit from sensor read")
            raise
        except Exception:
            Logger.crit("Exception with read=%s" % self.read_source)
        return data

    def write(self, value):
        if self.write_source is None:
            return

        try:
            self.write_func[self.write_type](self, value)
        except SystemExit:
            Logger.debug("SystemExit from sensor write")
            raise
        except Exception:
            Logger.crit("Exception with write=%s" % self.write_source)

    def write_kv(self, value):
        """
        Writes to write_source using kv.

        Arguments:
            value: value to be set to the sensor

        Return:
            N/A
        """

        try:
            kv_set(self.write_source, str(value * self.max_duty_register / 100))
        except Exception:
            raise

    def write_util(self, value):
        """
        Writes to write_source using util.

        Arguments:
            value: value to be set to the sensor

        Return:
            N/A
        """

        cmd = self.write_source % (int(value * self.max_duty_register / 100))
        Logger.debug("Setting value using cmd=%s" % cmd)
        try:
            response = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
            if response.find("Error") != -1:
                raise Exception("Write failed with response=%s" % response)
        except Exception:
            raise

    write_func = {
        'kv' : write_kv,
        'util' : write_util,
    }


class FscSensorSourceJson(FscSensorBase):
    """
    Class for FSC sensor source for JSON file
    """

    def get_filter(self):
        return self.filter

    def read(self, **kwargs):
        """
        Return:
            raw data read from json file
        """
        try:
            Logger.debug("Reading data with json file=%s" % self.read_source)
            with open(self.read_source, "r") as file:
                return file.read()

        except Exception as e:
            Logger.crit(
                "Exception while trying to read {read_source}: {exp}".format(
                    read_source=repr(self.read_source), exp=repr(e)
                )
            )
            return None
