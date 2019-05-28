#!/usr/bin/env python
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


class I2cSysfsUtils:
    _I2C_DEVICE_SYSFS_ROOT = "/sys/bus/i2c/devices"
    _I2C_DRIVER_SYSFS_ROOT = "/sys/bus/i2c/drivers"

    @classmethod
    def i2c_driver_dir(cls):
        return cls._I2C_DRIVER_SYSFS_ROOT

    @classmethod
    def i2c_device_dir(cls):
        return cls._I2C_DEVICE_SYSFS_ROOT

    @classmethod
    def i2c_driver_abspath(cls, driver):
        return os.path.join(cls._I2C_DRIVER_SYSFS_ROOT, driver)

    @classmethod
    def i2c_device_abspath(cls, device):
        return os.path.join(cls._I2C_DEVICE_SYSFS_ROOT, device)

    @classmethod
    def is_i2c_device_entry(cls, filename):
        """Check if the filename follows i2c device format:
           <bus>-<addr>.
        """
        pos = filename.find("-")
        if pos < 0:
            return False

        # I2C address part is decimal.
        if not filename[:pos].isdigit():
            return False

        # I2C device part is in hex format.
        try:
            int(filename[pos + 1 :], 16)
        except:
            return False

        return True

    @classmethod
    def is_i2c_bus_entry(cls, filename):
        """Check if the filename follows i2c driver format.
        """
        return filename.startswith("i2c-")

    @classmethod
    def i2c_device_get_driver(cls, device):
        """Get driver name of the given i2c device.
        """
        driver_name = ""

        device_dir = cls.i2c_device_abspath(device)
        if not os.path.isdir(device_dir):
            return ""
        for entry in os.listdir(device_dir):
            if entry == "driver":
                driver_path = os.path.join(device_dir, entry)
                if os.path.islink(driver_path):
                    target = os.readlink(driver_path)
                    driver_name = os.path.basename(target)

        return driver_name

    @classmethod
    def i2c_device_get_name(cls, device):
        device_name = ""

        device_dir = cls.i2c_device_abspath(device)
        name_file = os.path.join(device_dir, "name")
        if not os.path.exists(name_file):
            return ""

        with open(name_file, "r") as fp:
            lines = fp.readlines()
            if lines:
                device_name = lines[-1].strip()

        return device_name
