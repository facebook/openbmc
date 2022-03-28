import typing as t

import redfish_chassis_helper
import rest_pal_legacy
from aiohttp import web
from common_utils import dumps_bytestr
from redfish_base import validate_keys


def _get_fru_name_from_server_id(server_name: str) -> t.Optional[str]:
    "Returns the server name for an fru that their routes include"
    slot_name = None
    # if server_name is 1, we return None, as
    # get_fru_info_helper handles None as all non slot_ frus
    if server_name != "1":
        if rest_pal_legacy.pal_get_num_slots() > 1:
            if server_name.startswith("accelerator"):
                accel_index = int(server_name.replace("accelerator", ""))
                accelerators = redfish_chassis_helper._get_accelerator_list()
                return accelerators[accel_index]
            else:
                #  replace server with slot
                slot_name = server_name.replace("server", "slot")
        else:
            raise ValueError(
                "{server_name} server_name is invalid for slingle slot servers".format(
                    server_name=server_name
                )
            )
    return slot_name


async def get_chassis(request: web.Request) -> web.Response:
    members_json = redfish_chassis_helper.get_chassis_members_json()
    body = {
        "@odata.context": "/redfish/v1/$metadata#ChassisCollection.ChassisCollection",
        "@odata.id": "/redfish/v1/Chassis",
        "@odata.type": "#ChassisCollection.ChassisCollection",
        "Name": "Chassis Collection",
        "Members@odata.count": len(members_json),
        "Members": members_json,
    }
    await validate_keys(body)
    return web.json_response(body, dumps=dumps_bytestr)


async def get_chassis_member(request: web.Request) -> web.Response:
    server_name = request.match_info["fru_name"]
    try:
        fru_name = _get_fru_name_from_server_id(server_name)
    except ValueError:
        return web.json_response(status=404)
    frus_info_list = await redfish_chassis_helper.get_fru_info_helper(fru_name)
    fru = frus_info_list[0]
    if fru_name is None:
        fru_name = "Computer System Chassis"
    body = {
        "@odata.context": "/redfish/v1/$metadata#Chassis.Chassis",
        "@odata.id": "/redfish/v1/Chassis/{}".format(server_name),
        "@odata.type": "#Chassis.v1_15_0.Chassis",
        "Id": "1",
        "Name": fru_name,
        "ChassisType": "RackMount",
        "PowerState": "On",
        "Manufacturer": fru.manufacturer,
        "Model": fru.model,
        "SerialNumber": fru.serial_number,
        "Status": {"State": "Enabled", "Health": "OK"},
        "Sensors": {"@odata.id": "/redfish/v1/Chassis/{}/Sensors".format(server_name)},
        "Links": {"ManagedBy": [{"@odata.id": "/redfish/v1/Managers/1"}]},
    }
    await validate_keys(body)
    return web.json_response(body, dumps=dumps_bytestr)
