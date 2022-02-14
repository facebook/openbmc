import os
from functools import lru_cache
from shutil import which
from typing import List, Optional, TYPE_CHECKING

import common_utils
import rest_pal_legacy
from aiohttp import web
from redfish_base import RedfishError

POWER_UTIL_PATH = "/usr/local/bin/power-util"
WEDGE_POWER_PATH = "/usr/local/bin/wedge_power.sh"

# command: graceful-shutdown, off, on, reset, cycle, 12V-on, 12V-off, 12V-cycle
# Map each Redfish command with the relevant power_util command
# and expected status
POWER_UTIL_MAP = {
    "On": "on",
    "ForceOff": "12V-off",
    "GracefulShutdown": "graceful-shutdown",
    "GracefulRestart": "cycle",
    "ForceRestart": "12V-cycle",
    "ForceOn": "12V-on",
    "PushPowerButton": "reset",
}

# Commands:
#   status: Get the current power status

#   on: Power on microserver and main power if not powered on already
#     options:
#       -f: Re-do power on sequence no matter if microserver has
#           been powered on or not.

#   off: Power off microserver and main power ungracefully

#   reset: Power reset microserver ungracefully
#     options:
#       -s: Power reset whole wedge system ungracefully

WEDGE_POWER_MAP = {
    "On": "on",
    "ForceOff": "off",
    "GracefulShutdown": "off",  # ungraceful shutdown
    "GracefulRestart": "on -f",  # ungraceful
    "ForceRestart": "on -f",
    "ForceOn": "on",
    "PushPowerButton": "reset",
}


async def powercycle_post_handler(request: web.Request) -> web.Response:
    data = await request.json()
    fru_name = request.match_info["fru_name"]
    if rest_pal_legacy.pal_get_num_slots() > 1:
        server_name = fru_name.replace("server", "slot")
    allowed_resettypes = [
        "On",
        "ForceOff",
        "GracefulShutdown",
        "GracefulRestart",
        "ForceRestart",
        "ForceOn",
        "PushPowerButton",
    ]
    validation_error = await _validate_payload(request, allowed_resettypes)
    if validation_error:
        return RedfishError(
            status=400,
            message=validation_error,
        ).web_response()
    else:
        if is_power_util_available():  # then its compute
            reset_type = POWER_UTIL_MAP[data["ResetType"]]
            if fru_name is not None:
                cmd = [POWER_UTIL_PATH, server_name, reset_type]
                ret_code, resp, err = await common_utils.async_exec(cmd)
            else:
                raise NotImplementedError("Invalid URL")
        elif is_wedge_power_available():  # then its fboss
            reset_type = WEDGE_POWER_MAP[data["ResetType"]]
            cmd = [WEDGE_POWER_PATH, reset_type]
            ret_code, resp, err = await common_utils.async_exec(cmd)
        else:
            raise NotImplementedError("Util files not found")

    if ret_code == 0:
        return web.json_response(
            {"status": "OK", "Response": resp},
            status=200,
        )
    else:
        return RedfishError(
            status=500,
            message="Error changing {} system power state: {}".format(fru_name, err),
        ).web_response()


async def oobcycle_post_handler(request: web.Request) -> web.Response:
    allowed_resettypes = ["ForceRestart", "GracefulRestart"]
    validation_error = await _validate_payload(request, allowed_resettypes)
    if validation_error:
        return RedfishError(
            status=400,
            message=validation_error,
        ).web_response()
    else:
        if not TYPE_CHECKING:
            os.spawnvpe(
                os.P_NOWAIT, "sh", ["sh", "-c", "sleep 4 && reboot"], os.environ
            )
        return web.json_response(
            {"status": "OK"},
            status=200,
        )


async def _validate_payload(
    request: web.Request, allowed_types: List[str]
) -> Optional[str]:

    fru_name = request.match_info.get("fru_name")
    allowed_frus = ["1", "server1", "server2", "server3", "server4"]
    if fru_name and fru_name not in allowed_frus:
        return "fru_name:{} is not in the allowed fru list:{}".format(
            fru_name, allowed_frus
        )
    payload = await request.json()
    if "ResetType" not in payload:
        return "Reset Type Not Present in Payload"
    if payload["ResetType"] not in allowed_types:
        return "Submitted ResetType:{} is invalid. Allowed Resettypes:{}".format(
            payload["ResetType"], allowed_types
        )
    return None


@lru_cache()
def is_power_util_available() -> bool:
    return which(POWER_UTIL_PATH) is not None


@lru_cache()
def is_wedge_power_available() -> bool:
    return which(WEDGE_POWER_PATH) is not None
