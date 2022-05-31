import typing as t

from aiohttp import web
from common_utils import dumps_bytestr

VALID_KEYS = {
    "@odata.context",
    "@odata.id",
    "@odata.type",
    "Bios",
    "Id",
    "Name",
    "Product",
    "RedfishVersion",
    "UUID",
    "Systems",
    "Chassis",
    "Managers",
    "SessionService",
    "AccountService",
    "Description",
    "Accounts",
    "Members",
    "Members@odata.count",
    "ManagerType",
    "Status",
    "State",
    "Health",
    "FirmwareVersion",
    "NetworkProtocol",
    "EthernetInterfaces",
    "LogServices",
    "Links",
    "Actions",
    "target",
    "HostName",
    "FQDN",
    "#Manager.Reset",
    "ResetType@Redfish.AllowableValues",
    "InterfaceEnabled",
    "MACAddress",
    "SpeedMbps",
    "IPv4Addresses",
    "IPv6Addresses",
    "Address",
    "AddressOrigin",
    "StaticNameServers",
    "HTTP",
    "HTTPS",
    "SSH",
    "SSDP",
    "ProtocolEnabled",
    "Port",
    "Temperatures",
    "Fans",
    "Redundancy",
    "PowerControl",
    "ChassisType",
    "Manufacturer",
    "Model",
    "SerialNumber",
    "PowerState",
    "Thresholds",
    "FruInfo",
    "PhysicalContext",
    "ReadingRangeMin",
    "ReadingRangeMax",
    "ReadingUnits",
    "Reading",
    "Sensors",
    "ServiceEnabled",
    "SessionTimeout",
    "Sessions",
    "Oem",
    "Roles",
    "value",
    "name",
    "kind",
    "url",
    "NameServers",
    "LinkStatus",
    "DateTime",
    "DateTimeLocalOffset",
    "Entries",
    "LogEntryType",
    "MaxNumberOfRecords",
    "OverWritePolicy",
    "SyslogFilters",
    "LogFacilities",
    "LowestSeverity",
    "UpdateService",
    "EventTimestamp",
    "EntryType",
    "Message",
    "IndicatorLED",
}


async def validate_keys(body: t.Dict[str, t.Any]) -> None:
    required_keys = {"@odata.id", "@odata.type"}
    for required_key in required_keys:
        if required_key not in body:
            raise NotImplementedError(
                "required key: {required_key} not found in body: {body}".format(
                    required_key=repr(required_key), body=repr(body)
                )
            )
    for key, _value in body.items():
        if key not in VALID_KEYS:
            raise NotImplementedError(
                (
                    "key : {key} in response body : {body}"
                    " is not a valid RedFish key"
                ).format(  # noqa: B950
                    key=repr(key), body=repr(body)
                )
            )


class RedfishErrorExtendedInfo:
    def __init__(
        self,
        message_id: str = None,
        message: str = None,
        message_args: t.List[str] = None,
        severity: str = None,
        resolution: str = None,
    ):
        self.message_id = message_id
        self.message = message
        self.message_args = message_args
        self.severity = severity
        self.resolution = resolution

    def as_dict(self) -> t.Dict[str, t.Any]:
        result = {}  # type: t.Dict[str, t.Any]
        if self.message_id is not None:
            result["MessageId"] = self.message_id
        if self.message is not None:
            result["Message"] = self.message
        if self.message_args is not None:
            result["MessageArgs"] = self.message_args
        if self.severity is not None:
            result["Severity"] = self.severity
        if self.resolution is not None:
            result["Resolution"] = self.resolution
        return result


class RedfishError:
    def __init__(
        self,
        status: int,
        code: str = None,
        message: str = None,
        extended_info: t.List[RedfishErrorExtendedInfo] = None,
    ):
        self.status = status
        self.code = code
        self.message = message
        self.extended_info = extended_info

    def as_dict(self) -> t.Dict[t.Any, t.Any]:
        result = {}
        if self.code is not None:
            result["code"] = self.code
        if self.message is not None:
            result["message"] = self.message
        if self.extended_info is not None:
            result["@Message.ExtendedInfo"] = self._extended_info_as_dicts()
        return result

    def _extended_info_as_dicts(self):
        extended_info_as_dicts = []
        for item in self.extended_info:
            extended_info_as_dicts.append(item.as_dict())
        return extended_info_as_dicts

    def web_response(self) -> web.Response:
        return web.json_response(
            self.as_dict(), status=self.status, dumps=dumps_bytestr
        )
