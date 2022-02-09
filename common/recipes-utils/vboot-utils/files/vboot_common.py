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
import mmap
import os
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

VBS_LOCATION = {
    # VBS SRAM mapping: (addr , size, vbs_offset, vbs_size)
    "SOC_MODEL_ASPEED_G5": (0x1E720000, 4 * 1024, 0x200, 56),
    "SOC_MODEL_ASPEED_G6": (0x10015000, 4 * 1024, 0x800, 56),
}


def get_fdt(content):
    # Represent the FIT as an IO resource.
    if sys.version_info[0] == 3:
        fit_io = io.BytesIO(content)
    else:
        fit_io = StringIO.StringIO(content)
    dtb = pyfdt.FdtBlobParse(fit_io)
    fdt = dtb.to_fdt()
    return fdt


def get_fdt_from_file(infile):
    dtb = pyfdt.FdtBlobParse(infile, blob_limit=0x2000)
    fdt = dtb.to_fdt()
    return fdt


def get_soc_model():
    soc_model = "SOC_MODEL_UNKNOWN"
    with open("/etc/soc_model", "r") as fn:
        soc_model = fn.readline()

    return soc_model.strip()


def read_vbs() -> bytearray:
    memfn = None
    try:
        SRAM_OFFSET, SRAM_SIZE, VBS_OFFSET, VBS_SIZE = VBS_LOCATION[get_soc_model()]
        memfn = os.open("/dev/mem", os.O_RDWR | os.O_SYNC)
        with mmap.mmap(
            memfn,
            SRAM_SIZE,
            mmap.MAP_SHARED,
            mmap.PROT_READ | mmap.PROT_WRITE,
            offset=SRAM_OFFSET,
        ) as sram:
            sram.seek(VBS_OFFSET)
            return sram.read(VBS_SIZE)
    finally:
        if memfn is not None:
            os.close(memfn)
