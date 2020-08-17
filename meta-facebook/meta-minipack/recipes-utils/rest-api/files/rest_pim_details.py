#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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
import re
import subprocess
import typing as t
from collections import namedtuple

from aiohttp import web
from aiohttp.log import server_logger
from rest_utils import DEFAULT_TIMEOUT_SEC, dumps_bytestr


MAX_PIM_NUM = 8

PimFailure = namedtuple("PimFailure", ["status", "reason"])
PimData = namedtuple(
    "PimData",
    [
        "status",
        "version",
        "product_name",
        "product_part_number",
        "system_assembly_part_number",
        "facebook_pcba_part_number",
        "facebook_pcb_part_number",
        "odm_pcba_part_number",
        "odm_pcba_serial_number",
        "product_production_state",
        "product_version",
        "product_sub_version",
        "product_serial_number",
        "product_asset_tag",
        "system_manufacturer",
        "system_manufacturing_date",
        "pcb_manufacturer",
        "assembled_at",
        "local_mac",
        "extended_mac_base",
        "extended_mac_address_size",
        "location_on_fabric",
        "crc8",
    ],
)


def _extract_pimdata_from_peutil_result(stdout_data: t.List[str]) -> PimData:
    if re.match(
        "PIM[1-8] not inserted or could not detect eeprom", stdout_data.splitlines()[0]
    ):
        return PimFailure(status="missing", reason=stdout_data.splitlines()[0])
    initializer_dict = {"status": "present"}
    for line in stdout_data.splitlines()[1:]:  # discard Wedge EEPROM header
        parts = line.split(":")
        key = re.sub("[^a-z0-9]+", "_", parts[0].lower())
        initializer_dict[key] = parts[1].lstrip()
    return PimData(**initializer_dict)


class PimDetailResponse:
    def __init__(self):
        self.pimdatas = {}  # type: t.Dict[int, t.Union[PimData, PimFailure]]

    def addpimdata(self, pimid: int, pimdata: t.Union[PimData, PimFailure]):
        self.pimdatas[pimid] = pimdata

    def render(self):
        return web.json_response(
            {
                int(pim_id): pim_value._asdict()
                for (pim_id, pim_value) in self.pimdatas.items()
            },
            dumps=dumps_bytestr,
        )


def get_pim_details():
    response = PimDetailResponse()
    for pimid in range(1, MAX_PIM_NUM + 1):
        try:
            response.addpimdata(pimid, _execute_peutil(pimid))
        except Exception as ex:
            server_logger.exception(
                "Error getting pim data for pim: %d " % (pimid), exc_info=ex
            )
            response.addpimdata(pimid, PimFailure(status="unknown", reason=str(ex)))
    return response.render()


def _execute_peutil(pimid: int) -> t.Union[PimData, PimFailure]:
    cmd = ["/usr/local/bin/peutil", str(pimid)]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    return _extract_pimdata_from_peutil_result(data)
