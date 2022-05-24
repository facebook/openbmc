import os
import typing as t

import common_utils

import pal
import redfish_chassis_helper
import redfish_sensors
import rest_pal_legacy

from aiohttp import web
from redfish_base import validate_keys


class FruNotFoundError(Exception):
    pass


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
    expand_level = common_utils.parse_expand_level(request)
    members_json = await get_chassis_members_json(expand_level)
    body = {
        "@odata.context": "/redfish/v1/$metadata#ChassisCollection.ChassisCollection",
        "@odata.id": "/redfish/v1/Chassis",
        "@odata.type": "#ChassisCollection.ChassisCollection",
        "Name": "Chassis Collection",
        "Members@odata.count": len(members_json),
        "Members": members_json,
    }
    await validate_keys(body)
    return web.json_response(body, dumps=common_utils.dumps_bytestr)


def get_chassis_url_for_server_name(server_name: str) -> t.Dict[str, str]:
    return {"@odata.id": "/redfish/v1/Chassis/" + server_name}


async def _get_chassis_child_for_server_name(
    server_name: str, expand_level: int
) -> t.Dict[str, t.Any]:
    if expand_level > 0:
        return await get_chassis_member_for_fru(server_name, expand_level - 1)
    else:
        return get_chassis_url_for_server_name(server_name)


async def get_chassis_members_json(expand_level: int) -> t.List[t.Dict[str, t.Any]]:
    chassis_members = [await _get_chassis_child_for_server_name("1", expand_level)]
    # if libpal is supported, and slot4 is in the fru_name_map we are multi-slot
    if (
        redfish_chassis_helper.is_libpal_supported()
        and "slot4" in pal.pal_fru_name_map()
    ):
        # return chassis members for a multislot platform
        fru_name_map = pal.pal_fru_name_map()
        asics = 0
        for fru_name, fruid in fru_name_map.items():
            if pal.pal_is_fru_prsnt(fruid):
                child_name = None
                fru_capabilities = pal.pal_get_fru_capability(fruid)
                if (
                    pal.FruCapability.FRU_CAPABILITY_SERVER in fru_capabilities
                    and "slot" in fru_name
                ):
                    child_name = fru_name.replace("slot", "server")
                elif (
                    pal.FruCapability.FRU_CAPABILITY_HAS_DEVICE in fru_capabilities
                    and pal.FruCapability.FRU_CAPABILITY_SERVER not in fru_capabilities
                ):
                    child_name = "accelerator" + str(asics)
                    asics += 1
                if child_name:
                    chassis_members.append(
                        await _get_chassis_child_for_server_name(
                            child_name, expand_level
                        )
                    )
    return chassis_members


async def get_chassis_member(request: web.Request) -> web.Response:
    server_name = request.match_info["fru_name"]
    expand_level = common_utils.parse_expand_level(request)
    try:
        body = await get_chassis_member_for_fru(server_name, expand_level)
        return web.json_response(body, dumps=common_utils.dumps_bytestr)
    except FruNotFoundError:
        return web.json_response(status=404)


async def get_chassis_member_for_fru(
    server_name: str, expand_level: int
) -> web.Response:
    try:
        fru_name = _get_fru_name_from_server_id(server_name)
    except ValueError:
        raise FruNotFoundError()
    frus_info_list = await redfish_chassis_helper.get_fru_info_helper(fru_name)
    fru = frus_info_list[0]
    if expand_level > 0:
        sensors = await redfish_sensors.get_redfish_sensors_for_server_name(
            server_name, expand_level - 1
        )
    else:
        sensors = {"@odata.id": "/redfish/v1/Chassis/{}/Sensors".format(server_name)}
    if fru_name is None:
        fru_name = "Computer System Chassis"
    body = {
        "@odata.context": "/redfish/v1/$metadata#Chassis.Chassis",
        "@odata.id": "/redfish/v1/Chassis/{}".format(server_name),
        "@odata.type": "#Chassis.v1_15_0.Chassis",
        "Id": "1",
        "Name": fru_name,
        "ChassisType": "RackMount",
        "PowerState": await _get_chassis_power_state(fru_name),
        "Manufacturer": fru.manufacturer,
        "Model": fru.model,
        "SerialNumber": fru.serial_number,
        "Status": {"State": "Enabled", "Health": "OK"},
        "Sensors": sensors,
        "Links": {"ManagedBy": [{"@odata.id": "/redfish/v1/Managers/1"}]},
    }
    await validate_keys(body)
    return body


async def _get_chassis_power_state(fru_name: str) -> str:
    if os.path.exists("/usr/local/bin/wedge_power.sh"):
        return await _get_wedge_power_state()
    if os.path.exists("/usr/local/bin/power-util"):
        return await _get_compute_power_state(fru_name)
    return "On"


async def _get_wedge_power_state() -> str:
    cmd = ["/usr/local/bin/wedge_power.sh", " status"]
    _, stdout, _ = await common_utils.async_exec(cmd)
    return "On" if "is on" in stdout else "Off"


async def _get_compute_power_state(fru_name: str) -> str:
    if rest_pal_legacy.pal_get_num_slots() == 1:
        single_fru_name = get_single_fru_name()
        if single_fru_name is None:
            return ""
        else:
            fru_name = single_fru_name
    elif fru_name == "Computer System Chassis":
        return "On"
    cmd = ["/usr/local/bin/power-util", fru_name, "status"]
    _, stdout, _ = await common_utils.async_exec(cmd)
    return "On" if "ON" in stdout else "Off"


def get_single_fru_name() -> t.Optional[str]:
    fru_name_map = pal.pal_fru_name_map()
    for fru_name, fru_id in fru_name_map.items():
        if pal.FruCapability.FRU_CAPABILITY_SERVER in pal.pal_get_fru_capability(
            fru_id
        ):
            return fru_name
    return None
