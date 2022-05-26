import json
from shutil import which
from typing import Dict, List, Sequence

import common_utils
import pal
import rest_pal_legacy
from aiohttp import web
from redfish_base import validate_keys

LOG_UTIL_PATH = "/usr/local/bin/log-util"


class InvalidServerName(AssertionError):
    pass


class InvalidLogUtilResponse(RuntimeError):
    pass


class RedfishLogService:
    def get_log_service_controller(self) -> "RedfishLogServiceController":
        return RedfishLogServiceController(self)

    def get_server_name(self, fru_name: str) -> str:
        "Returns the server name for an fru that their routes include"
        server_name = "server"  # default for single sled frus

        if not hasattr(pal, "pal_fru_name_map"):
            return server_name

        frus = pal.pal_fru_name_map()
        slots = list(filter(lambda fru: "slot" in fru, frus))

        if len(slots) == 0:
            return server_name

        # We're on a multi slot device
        server_name = fru_name.replace("server", "slot")
        return server_name

    def is_log_util_available(self) -> bool:
        return which(LOG_UTIL_PATH) is not None

    async def _get_log_service_entries_sel(
        self, fru_name: str
    ) -> Sequence[Dict[str, str]]:
        log_service_id = "SEL"
        if fru_name == "1" and rest_pal_legacy.pal_get_num_slots() == 1:
            server_name = "all"
        else:
            server_name = self.get_server_name(fru_name)

        cmd = [LOG_UTIL_PATH, server_name, "--print", "--json"]
        ret, sel_entries, stderr = await common_utils.async_exec(cmd)
        if ret != 0:
            raise InvalidLogUtilResponse(stderr)

        # We need to load the initial JSON entries, then
        # modify the entries format to meet RedFish Schema
        raw_sel_entries = json.loads(sel_entries)
        redfish_sel_entries = []  # type: List[Dict[str, str]]

        # We need each entry to be aware of where it is,
        # and indexed by 1 per Redfish Schema
        index = 1
        for entry in raw_sel_entries["Logs"]:
            if entry["MESSAGE"].startswith("SEL Entry"):
                redfish_sel_entry = {}
                redfish_sel_entry[
                    "@odata.id"
                ] = "/redfish/v1/Systems/{}/LogServices/{}/Entries/{}".format(
                    fru_name, log_service_id, index
                )
                redfish_sel_entry["@odata.type"] = "#LogEntry.v1_10_0.LogEntry"
                redfish_sel_entry["EventTimestamp"] = entry["TIME_STAMP"]
                redfish_sel_entry["EntryType"] = entry["APP_NAME"]
                redfish_sel_entry["Message"] = entry["MESSAGE"].rstrip()
                redfish_sel_entry["Name"] = "{}:{}".format(
                    entry["FRU_NAME"], str(index)
                )
                redfish_sel_entries.append(redfish_sel_entry)
                index = index + 1
        return redfish_sel_entries

    async def get_logservices_root(self, request: web.Request) -> web.Response:
        members = []
        fru_name = request.match_info["fru_name"]
        if self.is_log_util_available:
            members.append(
                {"@odata.id": "/redfish/v1/Systems/{}/LogServices/SEL".format(fru_name)}
            )
        body = {
            "@odata.type": "#LogServiceCollection.LogServiceCollection",
            "Name": "Log Service Collection",
            "Description": "Collection of Logs for this System",
            "Members@odata.count": len(members),
            "Members": members,
            "Oem": {},
            "@odata.id": "/redfish/v1/Systems/{}/LogServices".format(fru_name),
        }

        return web.json_response(body)

    async def get_log_service(self, request: web.Request) -> web.Response:
        log_service_id = request.match_info["LogServiceID"]
        fru_name = request.match_info["fru_name"]

        # We only support log-util capable machines for now
        if not self.is_log_util_available:
            return web.Response(status=404)

        # We only support SEL right now
        if log_service_id.lower() != "sel":
            return web.Response(status=404)

        body = {
            "@odata.id": "/redfish/v1/Systems/{}/LogServices/{}".format(
                fru_name, log_service_id
            ),
            "@odata.type": "#LogService.v1_1_2.LogService",
            "Id": log_service_id,
            "LogEntryType": log_service_id,
            "Entries": {
                "@odata.id": "/redfish/v1/Systems/{}/LogServices/{}/Entries".format(
                    fru_name, log_service_id
                ),
            },
        }
        await validate_keys(body)
        return web.json_response(body)

    async def get_log_service_entries(self, request: web.Request) -> web.Response:
        log_service_id = request.match_info["LogServiceID"]
        fru_name = request.match_info["fru_name"]

        # We only support log-util capable machines for now
        if not self.is_log_util_available:
            return web.Response(status=404)

        if log_service_id.lower() == "sel":
            entries = await self._get_log_service_entries_sel(fru_name)
        else:
            # We only support SEL right now
            return web.Response(status=404)

        body = {
            "@odata.id": "/redfish/v1/Systems/{}/LogServices/{}/Entries".format(
                fru_name, log_service_id
            ),
            "@odata.type": "#LogEntryCollection.LogEntryCollection",
            "Description": "Collection of entries for this log service",
            "Members": entries,
            "Members@odata.count": len(entries),
        }
        await validate_keys(body)
        return web.json_response(body)

    async def get_log_service_entry(self, request: web.Request) -> web.Response:
        log_service_id = request.match_info["LogServiceID"]
        entry_id = request.match_info["EntryID"]
        fru_name = request.match_info["fru_name"]

        # We only support log-util capable machines for now
        if not self.is_log_util_available:
            return web.Response(status=404)

        if log_service_id.lower() == "sel":
            entries = await self._get_log_service_entries_sel(fru_name)
        else:
            # We only support SEL right now
            return web.Response(status=404)
        try:
            body = entries[int(entry_id) - 1]  # Index by 1 via schema
        except IndexError:
            return web.Response(status=404)
        print(body)
        await validate_keys(body)
        return web.json_response(body)


class RedfishLogServiceController:
    def __init__(self, handler: RedfishLogService):
        self.handler = handler

    async def get_log_services_root(self, request: web.Request) -> web.Response:
        return await self.handler.get_logservices_root(request)

    async def get_log_service(self, request: web.Request):
        return await self.handler.get_log_service(request)

    async def get_log_service_entries(self, request: web.Request):
        return await self.handler.get_log_service_entries(request)

    async def get_log_service_entry(self, request: web.Request):
        return await self.handler.get_log_service_entry(request)
