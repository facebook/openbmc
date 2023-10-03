#!/usr/local/bin/python3
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

import json
import os

from dataclasses import dataclass, asdict
from typing import List

"""
# JSON Specification
{
    "eeprom devices":
    [
        {
            "name": "eeprom",
            "sysfs_path": "/sys/bus/i2c/devices/xxx/eeprom"
        }
        ...
    ]
}
"""


@dataclass
class EepromSpec:
    name: str
    address: str


@dataclass
class EepromFileSpec:
    platform_name: str
    eeproms: List[EepromSpec]


def gen_eeprom(platname, data_file: str, output_dir: str):
    eeproms = parse_spec(data_file)
    fileSpec = EepromFileSpec(platform_name=platname, eeproms=eeproms)
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    output = os.path.join(output_dir, "eeprom.json")
    with open(output, "w+") as f:
        f.write(json.dumps(asdict(fileSpec), indent=4))


def get_eeprom_config(fname: str) -> List[EepromSpec]:
    try:
        with open(fname, "r") as f:
            return json.load(f)["eeprom devices"]
    except Exception as e:
        raise Exception(
            "Unable to read eeprom configuration file: {}".format(fname)) from e


def parse_spec(fname: str) -> List[EepromSpec]:
    eepromSpecs: List[EepromSpec] = []
    with open(fname, "r") as f:
        try:
            for parsed_eeprom in json.load(f)["eeprom devices"]:
                spec: EepromSpec = EepromSpec(name=parsed_eeprom["name"], address=parsed_eeprom["sysfs_path"])
                eepromSpecs.append(spec)
            return eepromSpecs

        except FileNotFoundError as e:
            raise Exception("Unable to find JSON file for eeprom specification") from e
        except json.decoder.JSONDecodeError as e:
            raise Exception("Unable to parse JSON for eeprom specification") from e
