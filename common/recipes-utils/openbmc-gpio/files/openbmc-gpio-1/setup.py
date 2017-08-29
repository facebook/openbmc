#!/usr/bin/env python3
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


from distutils.core import setup
from setup_board import board_py_modules

setup(
    name = 'openbmc-gpio',
    version = '1.0',
    description = 'OpenBMC GPIO utilities',
    author = 'Tian Fang',
    author_email = 'tfang@fb.com',
    license = 'GPLv2',
    py_modules=['openbmc_gpio',
                'openbmc_gpio_table',
                'phymemory',
                'soc_gpio',
                'soc_gpio_table',
                'board_gpio_table', ]
                + board_py_modules ,
)
