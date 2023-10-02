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

from typing import List

from jinja2 import Environment, FileSystemLoader, select_autoescape

TEMPLATE_DIR = "tools/fboss-lite/new_fblite_utils/templates"


def render_template(template_fname: str, **kwargs) -> str:
    env = Environment(
        loader=FileSystemLoader(TEMPLATE_DIR),
        autoescape=select_autoescape(),
    )
    template = env.get_template(template_fname)
    return template.render(kwargs)


def update_makefile(path: str, files_to_add: List[str]):
    target = "obj-m :="  # Search for this to know where to add
    lines_to_add = [f"obj-m += {fname}\n" for fname in files_to_add]
    lines = append_lines_at_target(path, target, lines_to_add)

    # Write the modified content back to the Makefile
    with open(path, "w") as makefile:
        makefile.writelines(lines)


def update_bb_file(path: str, files_to_add: List[str]):
    target = "LOCAL_URI"
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
