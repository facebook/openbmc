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

import re

from utils.cit_logger import Logger


def mac_verify(mac):
    """
    Helper method to verify if a MAC address is properly formatted
    """
    if re.match("[0-9a-f]{2}([:]?)[0-9a-f]{2}(\\1[0-9a-f]{2}){4}$", mac.lower()):
        return True
    return False


def read_data_from_filepath(path):
    """
    Helper method to read file from a file
    """
    try:
        handle = open(path, "r")
        data = handle.readline().rstrip()
        handle.close()
        Logger.debug("Reading path={} data={}".format(path, data))
        return data
    except Exception:
        raise ("Failed to read path={}".format(path))
