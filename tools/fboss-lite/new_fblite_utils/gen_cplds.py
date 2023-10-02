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

from new_fblite_utils.codegen_utils import (
    render_template,
    update_bb_file,
    update_makefile,
)

from jinja2 import Environment, FileSystemLoader, select_autoescape

CPLD_TEMPLATE = "cpld.c.jinja2"

"""
# JSON Specification
{
    cplds:
    [
        {
            name: "",
            attrs:
            [
                {
                    name: "",
                    read: true,
                    write: false,
                    register: "0x01", // json does not support hex numbers
                    offset: 0,
                    n_bits: 0,
                    help: "",
                },
                ...
            ],
            ...
        }
        ...
    ]
}
"""


@dataclass
class Attribute:
    name: str
    read: bool
    write: bool
    register: int
    offset: int
    n_bits: int
    help: Optional[str] = None


@dataclass
class CpldSpec:
    name: str
    attrs: List[Attribute]


def gen_cplds(data_file: str, kmod_dir: str):
    data = parse_spec(data_file)
    files_dir = os.path.join(kmod_dir, "files")
    for cpld in data:
        gen_cpld(cpld, files_dir)
    update_makefile(
        os.path.join(files_dir, "Makefile"), [cpld.name + ".o" for cpld in data]
    )
    update_bb_file(
        os.path.join(kmod_dir, "kernel-module-cpld_0.1.bb"),
        [cpld.name + ".c" for cpld in data],
    )


def gen_cpld(data: CpldSpec, output_dir: str):
    content = render_template(CPLD_TEMPLATE, name=data.name, attributes=data.attributes)
    output = os.path.join(output_dir, data.name + ".c")
    with open(output, "w") as f:
        f.write(content)


def parse_spec(fname: str) -> List[CpldSpec]:
    cplds: List[CpldSpec] = []
    try:
        with open(fname, "r") as f:
            for parsed_cpld in json.load(f)["cplds"]:
                attrs: List[Attribute] = []
                for attr in parsed_cpld.get("attrs"):
                    attr["register"] = int(attr["register"], 16)
                    attrs.append(Attribute(**attr))
                cplds.append(CpldSpec(parsed_cpld["name"], attrs))

    except FileNotFoundError as e:
        raise Exception("Unable to find JSON file for cpld specification") from e
    except json.decoder.JSONDecodeError as e:
        raise Exception("Unable to parse JSON for cpld specification") from e
    return cplds
