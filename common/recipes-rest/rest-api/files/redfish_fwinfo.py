import os
import typing as t
from collections import defaultdict

import common_utils
from aiohttp import web


async def get_update_service_root(request: web.Request) -> web.Response:
    body = {
        "@odata.type": "#UpdateService.v1_8_0.UpdateService",
        "@odata.id": "/redfish/v1/UpdateService",
        "Id": "UpdateService",
        "Name": "Update service",
        "Status": {"State": "Enabled", "Health": "OK", "HealthRollup": "OK"},
        "ServiceEnabled": False,
        "HttpPushUri": "/FWUpdate",
        "HttpPushUriOptions": {
            "HttpPushUriApplyTime": {
                "ApplyTime": "Immediate",
                "ApplyTime@Redfish.AllowableValues": [
                    "Immediate",
                    "OnReset",
                    "AtMaintenanceWindowStart",
                    "InMaintenanceWindowOnReset",
                ],
                "MaintenanceWindowStartTime": "2018-12-01T03:00:00+06:00",
                "MaintenanceWindowDurationInSeconds": 600,
            }
        },
        "HttpPushUriOptionsBusy": False,
        "FirmwareInventory": {
            "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory"
        },
        "Actions": {},
        "Oem": {},
    }
    return web.json_response(body)


async def get_firmware_inventory_root(request: web.Request) -> web.Response:
    expand_level = common_utils.parse_expand_level(request)
    fw_inventory_members = await _get_fw_inventory_members(expand_level=expand_level)
    body = {
        "@odata.type": "#SoftwareInventoryCollection.SoftwareInventoryCollection",
        "Name": "Firmware Collection",
        "Members@odata.count": len(fw_inventory_members),
        "Members": fw_inventory_members,
        "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory",
    }

    return web.json_response(body)


async def get_firmware_inventory_child(request: web.Request) -> web.Response:
    fw_name = request.match_info["fw_name"]
    fw_info_dict = await _gather_fw_info()
    try:
        return web.json_response(_render_fw_info_child(fw_name, fw_info_dict[fw_name]))
    except KeyError:
        return web.json_response(status=404)


async def _get_fw_inventory_members(expand_level: int) -> t.List[t.Dict[str, t.Any]]:
    fw_info_dict = await _gather_fw_info()
    members = []
    for fw_name, fw_version in fw_info_dict.items():
        if expand_level == 0:
            members.append(
                {
                    "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/{}".format(
                        fw_name
                    )
                }
            )
        else:
            members.append(_render_fw_info_child(fw_name, fw_version))
    return members


def _render_fw_info_child(fw_name: str, fw_ver: str) -> t.Dict[str, t.Any]:
    body = {
        "@odata.type": "#SoftwareInventory.v1_2_3.SoftwareInventory",
        "Id": fw_name,
        "Name": "{} Firmware".format(fw_name),
        "Status": {"State": "Enabled", "Health": "OK"},
        "Manufacturer": "Facebook",
        "Version": fw_ver,
        "SoftwareId": fw_ver,
        "Actions": {"Oem": {}},
        "Oem": {},
        "@odata.id": "/redfish/v1/UpdateService/FirmwareInventory/{}".format(fw_name),
    }
    return body


async def _gather_fw_info() -> t.Dict[str, t.Any]:
    fw_dict = await _execute_fwutil()
    fw_dict.update(await _execute_cpld_rev())
    fw_dict.update(await _execute_cpld_ver())
    fw_dict.update(await _execute_fpga_ver())
    return fw_dict


async def _execute_cpld_ver() -> t.Dict[str, t.Any]:
    if not os.path.exists("/usr/local/bin/cpld_ver.sh"):
        return {}
    cmd = ["/usr/local/bin/cpld_ver.sh"]
    results = {}
    retcode, stdout, err = await common_utils.async_exec(cmd, shell=False)
    for row in stdout.split("\n"):
        pieces = row.split(":")
        results[pieces[0]] = pieces[1]
    return results


async def _execute_cpld_rev() -> t.Dict[str, t.Any]:
    if not os.path.exists("/usr/local/bin/cpld_rev.sh"):
        return {}
    cmd = ["/usr/local/bin/cpld_rev.sh"]
    retcode, stdout, err = await common_utils.async_exec(cmd, shell=False)
    return {"CPLD": stdout}


async def _execute_fpga_ver() -> t.Dict[str, t.Any]:
    if not os.path.exists("/usr/local/bin/fpga_ver.sh"):
        return {}
    cmd = ["/usr/local/bin/fpga_ver.sh"]
    results = {}
    retcode, stdout, err = await common_utils.async_exec(cmd, shell=False)
    for row in stdout.split("\n"):
        if "--" not in row:
            pieces = row.split(":")
            if len(pieces) > 1:
                results[pieces[0].replace(" ", "_")] = pieces[1]
    return results


async def _execute_fwutil() -> t.Dict[str, t.Any]:
    cmd = ["/usr/bin/fw-util", "all", "--version"]
    retcode, stdout, err = await common_utils.async_exec(cmd, shell=False)
    gathered = defaultdict(list)
    for row in stdout.split("\n"):
        try:
            pieces = row.split(":")
            name = pieces[0].upper().replace("VERSION", "").strip().replace(" ", "_")
            val = pieces[1].strip()
            if (
                "NA" not in val
                and "Remaining" not in val
                and "AFTER_ACTIVATED" not in name
            ):
                gathered[name].append(val)
        except Exception:
            # Shell outputs can be wonky. Ignore failures
            pass
    formatted = {}
    for fw_names, fw_versions in gathered.items():
        if len(fw_versions) == 1:
            formatted[fw_names] = fw_versions[0]
        else:
            for idx, fw_ver in enumerate(fw_versions):
                formatted["SLOT{}_{}".format(idx + 1, fw_names)] = fw_ver
    return formatted
