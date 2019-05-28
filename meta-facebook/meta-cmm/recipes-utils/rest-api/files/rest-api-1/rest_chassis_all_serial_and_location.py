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

import json
import subprocess
import syslog
from collections import namedtuple

from backpack_cards import BACKPACK_ALL_CARDS


SnAndLocation = namedtuple("SnAndLocation", ("serial", "location"))


def _fork_procs_for_eeproms(card):
    procs = []
    api_fruid = "http://[{}]:8080/api/sys/mb/fruid".format(card.link_local_addr())
    api_fruid_scm = "{}_scm".format(api_fruid)
    apis = [api_fruid, api_fruid_scm] if card.has_scm else [api_fruid]
    for api in apis:
        procs.append(
            subprocess.Popen(
                ["/usr/bin/wget", "--timeout", "5", "-q", "-O", "-", api],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
        )
    return procs


def _get_serial_and_location(card, proc):
    data, err = proc.communicate()
    data = data.decode()
    if proc.returncode:
        raise Exception("Command failed, returncode: {}".format(proc.returncode))

    eeprom = json.loads(data)
    BASE_FRUID_FIELD = "Information"
    eeprom = eeprom[BASE_FRUID_FIELD] if BASE_FRUID_FIELD in eeprom else eeprom
    return SnAndLocation(
        serial=eeprom["Product Serial Number"].lower(),
        location=eeprom["Location on Fabric"].lower(),
    )


def _get_serials_and_loc_for_card(card, procs):
    bmc_serial_and_loc = _get_serial_and_location(card, procs[0])
    scm_serial_and_loc = (
        _get_serial_and_location(card, procs[1]) if card.has_scm else None
    )
    return (bmc_serial_and_loc, scm_serial_and_loc)


def _populate_serial_and_loc(slot_type, sn_and_loc):
    if not sn_and_loc:
        return {}
    slot_info = {}
    slot_info[slot_type] = {}
    for key in ["serial", "location"]:
        slot_info[slot_type][key] = getattr(sn_and_loc, key)
    return slot_info


def get_all_serials_and_locations():
    card_to_procs = {card: _fork_procs_for_eeproms(card) for card in BACKPACK_ALL_CARDS}
    card_infos = {}
    for card, procs in card_to_procs.items():
        try:
            serial_and_locs = _get_serials_and_loc_for_card(card, procs)
        except Exception as e:
            syslog.syslog(
                syslog.LOG_ERR,
                "Error getting SN, Location " "from {} error: {}".format(card, e),
            )
            continue

        card_key = str(card)
        card_infos[card_key] = {}

        for idx, slot_type in enumerate(["bmc", "scm"]):
            d = json.dumps(_populate_serial_and_loc(slot_type, serial_and_locs[idx]))
            card_infos[card_key].update(
                _populate_serial_and_loc(slot_type, serial_and_locs[idx])
            )

    return card_infos


if __name__ == "__main__":
    print("{}".format(get_all_serials_and_locations()))
