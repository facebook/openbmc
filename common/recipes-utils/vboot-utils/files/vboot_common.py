#!/usr/bin/env python
# Copyright 2020-present Facebook. All Rights Reserved.
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
import sys

from pyfdt import pyfdt


if sys.version_info[0] == 3:
    import io
else:
    import StringIO


# Define the exit code
EC_SUCCESS = 0
EC_SPLROM_BAD = 1
EC_UBOOT_BAD = 2
EC_KERNEL_BAD = 3
EC_ROOTFS_BAD = 4
EC_EXCEPTION = 255


def get_fdt(content):
    # Represent the FIT as an IO resource.
    if sys.version_info[0] == 3:
        fit_io = io.BytesIO(content)
    else:
        fit_io = StringIO.StringIO(content)
    dtb = pyfdt.FdtBlobParse(fit_io)
    fdt = dtb.to_fdt()
    return fdt
