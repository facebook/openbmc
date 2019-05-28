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

GPIOS = {
    "FAN_LED_BLUE": {"direction": "out", "active_low": "0", "value": "0"},
    "FCB3_CPLD_TDO": {"direction": "in", "active_low": "0", "value": "1"},
    "FCB1_CPLD_TMS": {"direction": "in", "active_low": "0", "value": "1"},
    "FCB4_CPLD_TCK": {"direction": "in", "active_low": "0", "value": "0"},
    "BMC_CPLD_TDO": {"direction": "in", "active_low": "0", "value": "1"},
    "BMC_CPLD_TCK": {"direction": "in", "active_low": "0", "value": "0"},
    "BMC_CPLD_TDI": {"direction": "in", "active_low": "0", "value": "0"},
    "FCB4_CPLD_TMS": {"direction": "in", "active_low": "0", "value": "1"},
    "FCB3_CPLD_TMS": {"direction": "in", "active_low": "0", "value": "1"},
    "FCB4_CPLD_TDO": {"direction": "in", "active_low": "0", "value": "1"},
    "SYS_LED_GREEN": {"direction": "in", "active_low": "0", "value": "1"},
    "PSU_LED_RED": {"direction": "in", "active_low": "0", "value": "1"},
    "FAN_LED_RED": {"direction": "in", "active_low": "0", "value": "1"},
    "PSU4_PRESENT": {"direction": "in", "active_low": "0", "value": "0"},
    "FCB1_CPLD_TDO": {"direction": "in", "active_low": "0", "value": "1"},
    "FCB3_CPLD_TDI": {"direction": "in", "active_low": "0", "value": "1"},
    "FAB_LED_GREEN": {"direction": "in", "active_low": "0", "value": "1"},
    "FCB3_CPLD_TCK": {"direction": "in", "active_low": "0", "value": "0"},
    "FCB1_CPLD_TCK": {"direction": "in", "active_low": "0", "value": "0"},
    "PSU2_PRESENT": {"direction": "in", "active_low": "0", "value": "0"},
    "PSU_LED_GREEN": {"direction": "in", "active_low": "0", "value": "1"},
    "FCB1_CPLD_TDI": {"direction": "in", "active_low": "0", "value": "1"},
    "FCB2_CPLD_TMS": {"direction": "in", "active_low": "0", "value": "1"},
    "FCB2_CPLD_TDO": {"direction": "in", "active_low": "0", "value": "1"},
    "FCB2_CPLD_TDI": {"direction": "in", "active_low": "0", "value": "1"},
    "FAB_LED_BLUE": {"direction": "out", "active_low": "0", "value": "0"},
    "PSU_LED_BLUE": {"direction": "out", "active_low": "0", "value": "0"},
    "FAN_LED_GREEN": {"direction": "in", "active_low": "0", "value": "1"},
    "PSU1_PRESENT": {"direction": "in", "active_low": "0", "value": "0"},
    "FCB4_CPLD_TDI": {"direction": "in", "active_low": "0", "value": "1"},
    "SYS_LED_RED": {"direction": "in", "active_low": "0", "value": "1"},
    "FCB2_CPLD_TCK": {"direction": "in", "active_low": "0", "value": "0"},
    "SYS_LED_BLUE": {"direction": "out", "active_low": "0", "value": "0"},
    "FAB_LED_RED": {"direction": "in", "active_low": "0", "value": "1"},
    "PSU3_PRESENT": {"direction": "in", "active_low": "0", "value": "0"},
    "BMC_CPLD_TMS": {"direction": "in", "active_low": "0", "value": "0"},
    "CPLD_JTAG_SEL": {"direction": "in", "active_low": "0", "value": "1"},
}
