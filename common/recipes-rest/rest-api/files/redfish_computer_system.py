import pal
from aiohttp import web
from common_utils import dumps_bytestr
from redfish_base import validate_keys


class RedfishComputerSystems:
    def __init__(self):
        self.collection = []
        if not hasattr(pal, "pal_fru_name_map"):
            return
        fru_name_map = pal.pal_fru_name_map()
        for key, fruid in fru_name_map.items():
            if (
                hasattr(pal, "FruCapability")
                and pal.FruCapability.FRU_CAPABILITY_SERVER
                not in pal.pal_get_fru_capability(fruid)
            ) or (not hasattr(pal, "FruCapability") and key.find("slot") != 0):
                continue

            server_name = key.replace("slot", "server")
            self.collection.append(
                {
                    "@odata.id": "/redfish/v1/Systems/{}".format(server_name),
                    "Name": server_name,
                }
            )

    async def get_collection_descriptor(self, request: web.Request) -> web.Response:
        body = {
            "@odata.id": "/redfish/v1/Systems",
            "@odata.context": "/redfish/v1/$metadata#ComputerSystemCollection.ComputerSystemCollection",
            "@odata.type": "#ComputerSystemCollection.ComputerSystemCollection",
            "Name": "Computer System Collection",
            "Members@odata.count": len(self.collection),
            "Members": self.collection,
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    def has_server(self, server_name) -> bool:
        for server in self.collection:
            if server["Name"] == server_name:
                return True
        return False

    async def get_system_descriptor(
        self, server_name: str, request: web.Request
    ) -> web.Response:
        if not self.has_server(server_name):
            return web.Response(status=404)

        body = {
            "@odata.id": "/redfish/v1/Systems/{}".format(server_name),
            "@odata.type": "#ComputerSystem.ComputerSystem",
            "Name": server_name,
            "Bios": {
                "@odata.id": "/redfish/v1/Systems/{}/Bios".format(server_name),
            },
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
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    async def get_bios_descriptor(
        self, server_name: str, request: web.Request
    ) -> web.Response:
        if not self.has_server(server_name):
            return web.Response(status=404)

        body = {
            "@odata.id": "/redfish/v1/Systems/{}/Bios".format(server_name),
            "@odata.type": "#Bios.Bios",
            "Name": "{} BIOS".format(server_name),
            "Links": {
                "ActiveSoftwareImage": {
                    "@odata.id": "/redfish/v1/Systems/{}/Bios/FirmwareInventory".format(
                        server_name,
                    )
                },
            },
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    async def get_bios_firmware_inventory(self, server_name: str, request: web.Request):
        if not self.has_server(server_name):
            return web.Response(status=404)

        body = {
            "@odata.id": "/redfish/v1/Systems/{}/Bios/FirmwareInventory".format(
                server_name
            ),
            "@odata.type": "#SoftwareInventory.SoftwareInventory",
            "Id": "{}/Bios".format(server_name),
            "Name": "{} BIOS Firmware".format(server_name),
            "Oem": {
                "Dumps": {
                    "@odata.id": "/redfish/v1/Systems/{}/Bios/FirmwareDumps".format(
                        server_name
                    ),
                }
            },
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    def get_server(self, server_name: str) -> "RedfishComputerSystem":
        return RedfishComputerSystem(self, server_name=server_name)


class RedfishComputerSystem:
    def __init__(self, collection: RedfishComputerSystems, server_name: str):
        self.collection = collection
        self.server_name = server_name

    async def get_system_descriptor(self, request: web.Request):
        return await self.collection.get_system_descriptor(self.server_name, request)

    async def get_bios_descriptor(self, request: web.Request):
        return await self.collection.get_bios_descriptor(self.server_name, request)

    async def get_bios_firmware_inventory(self, request: web.Request):
        return await self.collection.get_bios_firmware_inventory(
            self.server_name, request
        )
