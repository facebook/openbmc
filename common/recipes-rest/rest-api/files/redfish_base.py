import typing as t

VALID_KEYS = {
    "@odata.context",
    "@odata.id",
    "@odata.type",
    "Id",
    "Name",
    "Product",
    "RedfishVersion",
    "UUID",
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
                "key : {key} in response body : {body} is not a valid RedFish key".format(  # noqa: B950
                    key=repr(key), body=repr(body)
                )
            )
