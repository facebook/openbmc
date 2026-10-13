#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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

"""Trim the weutil config down to the EEPROMs fitted on this unit.

nexthopbmc hardware revisions differ in which FRU EEPROMs are populated:
eeprom-devices.json lists every EEPROM the platform can have, and the entries
whose sysfs node is missing are dropped so weutil only reports the EEPROMs
setup_i2c.sh managed to instantiate.

chassis_eeprom is weutil's built-in default device, so everything running
bare weutil (dhcp-id, rest-api fruid, tests) depends on it. Until the
chassis EEPROM is populated on all hardware, units without one alias
chassis_eeprom to the BMC EEPROM, which carries the chassis identity data
(serial, MACs) in the meantime.
"""

import json
import os
import sys

DEVICES_FILE = "/etc/weutil/eeprom-devices.json"
CONFIG_FILE = "/etc/weutil/eeprom.json"

CHASSIS_EEPROM = "chassis_eeprom"
CHASSIS_FALLBACK = "bmc_eeprom"


def main():
    with open(DEVICES_FILE) as fp:
        config = json.load(fp)

    devices = []
    for device in config["eeprom devices"]:
        if os.path.exists(device["sysfs_path"]):
            devices.append(device)
        else:
            print(
                "%s is missing, skipping %s."
                % (device["sysfs_path"], device["name"])
            )

    names = {d["name"]: d for d in devices}
    if CHASSIS_EEPROM not in names and CHASSIS_FALLBACK not in names:
        sys.exit(
            "Neither %s nor %s is present, not writing %s."
            % (CHASSIS_EEPROM, CHASSIS_FALLBACK, CONFIG_FILE)
        )
    if CHASSIS_EEPROM not in names and CHASSIS_FALLBACK in names:
        print("%s not fitted, aliasing to %s." % (CHASSIS_EEPROM, CHASSIS_FALLBACK))
        devices.insert(
            0,
            {
                "name": CHASSIS_EEPROM,
                "sysfs_path": names[CHASSIS_FALLBACK]["sysfs_path"],
            },
        )

    config["eeprom devices"] = devices
    with open(CONFIG_FILE, "w") as fp:
        json.dump(config, fp, indent=2)
        fp.write("\n")

    print("Configured EEPROMs: %s" % ", ".join(d["name"] for d in devices))


if __name__ == "__main__":
    main()
