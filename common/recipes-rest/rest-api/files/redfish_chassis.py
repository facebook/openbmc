import typing as t

import redfish_chassis_helper
from aiohttp import web
from common_utils import dumps_bytestr
from redfish_base import validate_keys


class RedfishChassis:
    fru_name = None

    def __init__(self, fru_name: t.Optional[str] = None):
        self.fru_name = fru_name
        # if fru_name is None, we retrieve frus for single sled devices

    def get_server_name(self):
        "Returns the server name for an fru that their routes include"
        server_name = "1"  # default for single sled frus
        if self.fru_name is not None:
            #  replace slot with server
            server_name = self.fru_name.replace("slot", "server")
        return server_name

    async def get_chassis(self, request: str) -> web.Response:
        members_json = redfish_chassis_helper.get_chassis_members_json()
        body = {
            "@odata.context": "/redfish/v1/$metadata#ChassisCollection.ChassisCollection",  # noqa: B950
            "@odata.id": "/redfish/v1/Chassis",
            "@odata.type": "#ChassisCollection.ChassisCollection",
            "Name": "Chassis Collection",
            "Members@odata.count": len(members_json),
            "Members": members_json,
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    async def get_chassis_members(self, request: str) -> web.Response:
        server_name = self.get_server_name()
        frus_info_list = await redfish_chassis_helper.get_fru_info_helper(self.fru_name)
        fru = frus_info_list[0]
        body = {
            "@odata.context": "/redfish/v1/$metadata#Chassis.Chassis",
            "@odata.id": "/redfish/v1/Chassis/{}".format(server_name),
            "@odata.type": "#Chassis.v1_15_0.Chassis",
            "Id": "1",
            "Name": "Computer System Chassis",
            "ChassisType": "RackMount",
            "PowerState": "On",
            "Manufacturer": fru.manufacturer,
            "Model": fru.model,
            "SerialNumber": fru.serial_number,
            "Status": {"State": "Enabled", "Health": "OK"},
            "Sensors": {
                "@odata.id": "/redfish/v1/Chassis/{}/Sensors".format(server_name)
            },
            "Links": {
                "ManagedBy": [
                    {"@odata.id": "/redfish/v1/Chassis/{}".format(server_name)}
                ]
            },
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)
