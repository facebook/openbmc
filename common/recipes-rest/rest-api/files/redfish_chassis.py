import functools
import os
import typing as t

import common_utils

import kv
import pal
import redfish_chassis_helper
import redfish_sensors
import rest_pal_legacy

from aiohttp import web
from aiohttp.log import server_logger
from redfish_base import validate_keys


class FruNotFoundError(Exception):
    pass


KV_IDENTIFY_PLAT_KEY_MAP_SINGLE_SLOT_SPECIAL = {
    "fbttn": "identify_slot1",
    "grandcanyon": "system_identify_server",
}
KV_IDENTIFY_SINGLES_LOT_DEFAULT = "identify_sled"


@functools.lru_cache(maxsize=1)
def _get_platform_name() -> str:
    machine = ""
    with open("/etc/issue") as f:
        line = f.read().strip()
        if line.startswith("OpenBMC Release "):
            tmp = line.split(" ")
            vers = tmp[2]
            tmp2 = vers.split("-")
            machine = tmp2[0]
        return machine


def _get_slot_led_state(slot: str) -> str:
    if not os.path.exists("/usr/bin/fpc-util"):
        # TODO : Figure out how front panel LED status is stored on WEDGE if at all.
        return "Unknown"
    if rest_pal_legacy.pal_get_num_slots() > 1:
        if slot == "Computer System Chassis":
            slot_led_state_key = KV_IDENTIFY_SINGLES_LOT_DEFAULT
        else:
            slot_led_state_key = "identify_{}".format(slot)
    else:
        plat_name = _get_platform_name()
        slot_led_state_key = KV_IDENTIFY_PLAT_KEY_MAP_SINGLE_SLOT_SPECIAL.get(
            plat_name, KV_IDENTIFY_SINGLES_LOT_DEFAULT
        )
    slot_led_state = _get_kv_persistent(slot_led_state_key)
    if slot_led_state == "on":
        return "Lit"
    elif slot_led_state == "off":
        return "Off"
    else:
        return "Unknown"


def _get_kv_persistent(key: str) -> str:
    try:
        return kv.kv_get(key, kv.FPERSIST)
    except kv.KeyOperationFailure:
        return ""


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
) -> t.Dict[str, t.Any]:
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
        "IndicatorLED": _get_slot_led_state(fru_name),
        "Manufacturer": fru.manufacturer,
        "Model": fru.model,
        "SerialNumber": fru.serial_number,
        "Status": {"State": "Enabled", "Health": "OK"},
        "Sensors": sensors,
        "Links": {"ManagedBy": [{"@odata.id": "/redfish/v1/Managers/1"}]},
    }
    await validate_keys(body)
    return body


async def set_power_state(request: web.Request) -> web.Response:
    server_name = request.match_info["fru_name"]
    payload = await request.json()
    led_desired_state = payload.get("IndicatorLED", "")
    if led_desired_state not in ["Lit", "Off"]:
        return web.json_response(
            data={
                "status": "Bad Request",
                "reason": "{} is invalid IndicatorLED state, valid states are: Lit, Off".format(
                    led_desired_state
                ),
            },
            status=400,
        )
    try:
        fru_name = _get_fru_name_from_server_id(server_name)
    except ValueError:
        raise FruNotFoundError()
    if fru_name is None:
        fru_name = "sled"
    led_desired_state = "on" if led_desired_state == "Lit" else "off"
    success = await _change_led_state_for_fru(fru_name, led_desired_state)
    return web.json_response({"success": success})


async def _change_led_state_for_fru(fru_name: str, target_state: str) -> bool:
    if not os.path.exists("/usr/bin/fpc-util"):
        raise NotImplementedError(
            "Front panel status change is not implemented for Network platforms"
        )
    if rest_pal_legacy.pal_get_num_slots() > 1:
        if _get_platform_name() == "fby3" and fru_name == "sled":
            # FBY3 does not expose "sled" level LED via fpc-util
            raise NotImplementedError("fby3 does not support sled level LED")
        cmd = ["/usr/bin/fpc-util", fru_name, "--identify", target_state]
    else:
        if _get_platform_name() == "fbtp":
            # Tiogapass is the only slingle slot compute system that needs the "sled" identifier in the args list
            cmd = ["/usr/bin/fpc-util", "sled", "--identify", target_state]
        else:
            cmd = ["/usr/bin/fpc-util", "--identify", target_state]

    exit_code, stdout, stderr = await common_utils.async_exec(cmd)
    if exit_code != 0:
        server_logger.error(
            "Failed to change fpc LED status, cmd={}, stdout={}, stderr={}".format(
                repr(cmd), repr(stdout), repr(stderr)
            )
        )
    if exit_code == 0 and _get_platform_name() == "fby3":
        # XXX: Yv3 FPC util does not set status in kv store, so we do it here.
        kv.kv_set("identify_{}".format(fru_name), target_state, kv.FPERSIST)

    return exit_code == 0


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
