# Copyright 2015-present Facebook. All rights reserved.
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
from __future__ import absolute_import, division, print_function, unicode_literals

import phymemory


_soc_reg_map = {}


class SCUReg(phymemory.PhyMemory):
    SCU_REG_MAX = 0x1A4
    SCU_ADDR_BASE = 0x1E6E2000

    def __init__(self, reg):
        assert 0 <= reg <= self.SCU_REG_MAX
        self.reg = reg
        super(SCUReg, self).__init__(self.SCU_ADDR_BASE + reg, "SCU%X" % reg)


def soc_get_register(addr):
    if addr in _soc_reg_map:
        return _soc_reg_map[addr]
    reg = SCUReg(addr)
    _soc_reg_map[addr] = reg
    return reg


def soc_get_tolerance_reg(addr):
    if addr in _soc_reg_map:
        return _soc_reg_map[addr]
    reg = phymemory.PhyMemory(addr)
    _soc_reg_map[addr] = reg
    return reg
