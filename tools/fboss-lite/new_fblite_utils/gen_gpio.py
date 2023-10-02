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

from dataclasses import dataclass
from typing import List, Optional

from jinja2 import Environment, FileSystemLoader, select_autoescape

GPIO_TEMPLATE = "setup-gpio.sh.jinja2"
FILE_NAME = "setup-gpio.sh"

"""
JSON Specification
{
    gpio:
    [
        {
            comment: "", // optional
            name: "",
            pin: "",
            direction: "", // in|out
            default: 0 // 0|1 - optional
        },
        {
            ...
        }
    ]
}
"""


@dataclass
class Gpio:
    name: str
    pin: str
    direction: str
    default: Optional[int] = None
    comment: Optional[str] = None


@dataclass
class GpioSpec:
    gpios: List[Gpio]


def gen_gpio(data_file: str, util_dir: str):
    data = parse_spec(data_file)
    files_dir = os.path.join(util_dir, "files")
    content = render_template(GPIO_TEMPLATE, data)
    output = os.path.join(files_dir, FILE_NAME)
    with open(output, "w") as f:
        f.write(content)
    update_bb_file(
        os.path.join(util_dir, "openbmc-utils_%.bbappend"), [FILE_NAME]
    )  # TODO: Implement generally


def parse_spec(fname: str) -> GpioSpec:
    gpios: List[Gpio] = []
    try:
        with open(fname, "r") as f:
            parsed_gpio_list = json.load(f)["gpio"]
            for gpio_data in parsed_gpio_list:
                if gpio_data["direction"] not in ["in", "out"]:
                    raise Exception(
                        f"Invalid direction for gpio pin: {gpio_data['direction']}"
                    )
                gpios.append(Gpio(**gpio_data))
    except FileNotFoundError as e:
        raise Exception("Unable to find JSON file for cpld specification") from e
    except json.decoder.JSONDecodeError as e:
        raise Exception("Unable to parse JSON for cpld specification") from e
    return GpioSpec(gpios=gpios)


def render_template(template_fname: str, data: GpioSpec) -> str:
    env = Environment(
        loader=FileSystemLoader("tools/fboss-lite/new_fblite_utils/templates"),
        autoescape=select_autoescape(),
    )
    template = env.get_template(template_fname)
    return template.render(gpios=data.gpios)


def update_bb_file(path: str, files_to_add: List[str]):
    target = "LOCAL_URI += "
    lines_to_add = [f"    file://{fname} \\\n" for fname in files_to_add]
    lines = append_lines_at_target(path, target, lines_to_add)

    # sort the file URIs alphabetically
    start_uri_index = next((i for i, line in enumerate(lines) if target in line), None)
    end_uri_index = (
        next(
            (i for i, line in enumerate(lines[start_uri_index + 1 :]) if '"' in line),
            None,
        )
        + start_uri_index
    )

    uri_lines = lines[start_uri_index + 1 : end_uri_index]
    uri_lines.sort()
    lines[start_uri_index + 1 : end_uri_index] = uri_lines

    with open(path, "w") as bb_file:
        bb_file.writelines(lines)


def append_lines_at_target(filename: str, target: str, lines_to_add: List[str]):
    with open(filename, "r") as makefile:
        lines = makefile.readlines()

    # Locate the target line and insert new lines
    index_to_insert = next((i for i, line in enumerate(lines) if target in line), None)
    if index_to_insert is not None:
        index_to_insert += 1
        for line in lines_to_add:
            lines.insert(index_to_insert, line)
    return lines
