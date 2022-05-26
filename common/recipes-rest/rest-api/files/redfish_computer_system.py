import os
from typing import List

import pal
import rest_pal_legacy
from aiohttp import web
from common_utils import dumps_bytestr
from redfish_base import validate_keys


def pal_supported() -> bool:
    UNSUPPORTED_PLATFORM_BUILDNAMES = ["yamp", "wedge", "wedge100"]
    if not hasattr(pal, "pal_get_platform_name"):
        return False
    return pal.pal_get_platform_name() not in UNSUPPORTED_PLATFORM_BUILDNAMES


def sel_supported() -> bool:
    LOG_UTIL_PATH = "/usr/local/bin/log-util"
    return os.path.exists(LOG_UTIL_PATH)


def get_compute_system_names() -> List[str]:
    if not pal_supported() or rest_pal_legacy.pal_get_num_slots() == 1:
        return ["1"]

    if not hasattr(pal, "pal_fru_name_map"):
        return [""]
    fru_name_map = pal.pal_fru_name_map()

    # The fallback way
    if not hasattr(pal, "FruCapability") or not hasattr(pal, "pal_get_fru_capability"):
        compute_system_names = []
        for key, _ in fru_name_map.items():
            if key.find("slot") != 0:
                continue
            server_name = key.replace("slot", "server")
            compute_system_names.append(server_name)
        return compute_system_names

    # The right way
    compute_system_names = []
    for key, fruid in fru_name_map.items():
        if pal.FruCapability.FRU_CAPABILITY_SERVER not in pal.pal_get_fru_capability(
            fruid
        ):
            continue
        server_name = key.replace("slot", "server")
        compute_system_names.append(server_name)
    return compute_system_names


class RedfishComputerSystems:
    def __init__(self):
        self.collection = []
        for system_name in get_compute_system_names():
            self.collection.append(
                {
                    "@odata.id": "/redfish/v1/Systems/{}".format(system_name),
                    "Name": system_name,
                }
            )

    async def get_collection_descriptor(self, request: web.Request) -> web.Response:
        headers = {
            "Link": "</redfish/v1/schemas/v1/ComputerSystemCollection.json>; rel=describedby"  # noqa: E501
        }
        body = {
            "@odata.id": "/redfish/v1/Systems",
            "@odata.context": "/redfish/v1/$metadata#ComputerSystemCollection.ComputerSystemCollection",  # noqa: E501
            "@odata.type": "#ComputerSystemCollection.ComputerSystemCollection",
            "Name": "Computer System Collection",
            "Members@odata.count": len(self.collection),
            "Members": self.collection,
        }
        await validate_keys(body)
        return web.json_response(body, headers=headers, dumps=dumps_bytestr)

    def has_server(self, server_name) -> bool:
        for server in self.collection:
            if server["Name"] == server_name:
                return True
        return False

    async def get_system_descriptor(self, request: web.Request) -> web.Response:
        server_name = request.match_info["server_name"]
        if not self.has_server(server_name):
            return web.Response(status=404)

        body = {
            "@odata.id": "/redfish/v1/Systems/{}".format(server_name),
            "@odata.type": "#ComputerSystem.v1_16_1.ComputerSystem",
            "Id": server_name,
            "Name": server_name,
            "Actions": {
                "#ComputerSystem.Reset": {
                    "target": "/redfish/v1/Systems/{}/Actions/ComputerSystem.Reset".format(  # noqa B950
                        server_name
                    ),
                    "ResetType@Redfish.AllowableValues": [
                        "On",
                        "ForceOff",
                        "GracefulShutdown",
                        "GracefulRestart",
                        "ForceRestart",
                        "ForceOn",
                        "PushPowerButton",
                    ],
                },
            },
        }
        if pal_supported():
            body["Bios"] = {
                "@odata.id": "/redfish/v1/Systems/{}/Bios".format(server_name),
            }
        if sel_supported():
            body["LogServices"] = {
                "@odata.id": "/redfish/v1/Systems/{}/LogServices".format(server_name)
            }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    async def get_bios_descriptor(self, request: web.Request) -> web.Response:
        server_name = request.match_info["server_name"]
        if not self.has_server(server_name):
            return web.Response(status=404)

        body = {
            "@odata.id": "/redfish/v1/Systems/{}/Bios".format(server_name),
            "@odata.type": "#Bios.v1_1_0.Bios",
            "Name": "{} BIOS".format(server_name),
            "Id": "{} BIOS".format(server_name),
            "Links": {
                "ActiveSoftwareImage": {
                    "@odata.id": "/redfish/v1/Systems/{}/Bios/FirmwareInventory".format(
                        server_name,
                    ),
                    "@odata.type": "#SoftwareInventory.v1_0_0.SoftwareInventory",
                    "Id": "{}/Bios".format(server_name),
                    "Name": "{} BIOS Firmware".format(server_name),
                },
            },
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    async def get_bios_firmware_inventory(self, request: web.Request):
        server_name = request.match_info["server_name"]

        if not self.has_server(server_name):
            return web.Response(status=404)

        body = {
            "@odata.id": "/redfish/v1/Systems/{}/Bios/FirmwareInventory".format(
                server_name
            ),
            "@odata.type": "#SoftwareInventory.v1_0_0.SoftwareInventory",
            "Id": "{}/Bios".format(server_name),
            "Name": "{} BIOS Firmware".format(server_name),
            "Oem": {
                "Dumps": {
                    "@odata.type": "#FirmwareDumps.v1_0_0.FirmwareDumps",
                    "@odata.context": "/redfish/v1/$metadata#FirmwareDumps.FirmwareDumps",
                    "@odata.id": "/redfish/v1/Systems/{}/Bios/FirmwareDumps".format(
                        server_name
                    ),
                    "Id": "{}/Bios Dumps".format(server_name),
                    "Name": "{}/Bios Dumps".format(server_name),
                }
            },
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)
