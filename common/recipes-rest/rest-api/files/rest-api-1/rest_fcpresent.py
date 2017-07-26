#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

from rest_helper import read_gpio_sysfs, get_wedge_slot

def is_mon_fc_present():
    # read monitored PEER FC : GPIOU7 =  GPIO167
    if (read_gpio_sysfs(167) == 1):
        return 0 #removed
    elif (read_gpio_sysfs(167) == 0):
        return 1 #present
    return None

def get_fcpresent():
    slot = get_wedge_slot()
    mon_slot = 0
    FC_CARD_BASE = 100

    # FC101 top (101,102)
    # FC201 bottom (201,202)
    if ((slot == 101) or (slot == 102)):
        mon_slot = slot + FC_CARD_BASE

    if ((slot == 201) or (slot == 202)):
        mon_slot = slot - FC_CARD_BASE

    if ( mon_slot ):
        if is_mon_fc_present() == None:
            status = 'Not Applicable' # gpio read failed
        elif is_mon_fc_present():
            status = "Present"
        else:
            status = "Removed"
    else:
        status = 'Not Applicable' # If this was a LC

    result = {
                "Information": {
                    "Slotid": slot,
                    "Monitored Slotid": mon_slot,
                    "Status": status,
                    "Description" : "Slotid indicates monitored slotid's status"
                },
                "Actions": [],
                "Resources": [],
             }
    return result
