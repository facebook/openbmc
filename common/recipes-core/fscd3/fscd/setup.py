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


from setuptools import setup


setup(
    name="fscd",
    version="1.0",
    description="OpenBMC FSCD Utility",
    license="GPLv2",
    scripts=["fscd.py"],
    py_modules=[
        "fsc_bmcmachine",
        "fsc_common_var",
        "fsc_control",
        "fsc_expr",
        "fsc_parser",
        "fsc_profile",
        "fsc_sensor",
        "fsc_util",
        "fsc_zone",
        "fsc_board",
    ],
)
