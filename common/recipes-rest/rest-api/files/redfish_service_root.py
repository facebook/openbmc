import rest_pal_legacy
from aiohttp import web
from common_utils import dumps_bytestr
from redfish_base import validate_keys


async def get_redfish(request: str) -> web.json_response:
    body = {"v1": "/redfish/v1"}
    return web.json_response(body, dumps=dumps_bytestr)


async def get_service_root(request: str) -> web.json_response:
    product_name = str(rest_pal_legacy.pal_get_platform_name())
    uuid_data = str(rest_pal_legacy.pal_get_uuid())
    body = {
        "@odata.context": "/redfish/v1/$metadata#ServiceRoot.ServiceRoot",
        "@odata.id": "/redfish/v1/",
        "@odata.type": "#ServiceRoot.v1_3_0.ServiceRoot",
        "Id": "RootService",
        "Name": "Root Service",
        "Product": product_name,
        "RedfishVersion": "1.0.0",
        "UUID": uuid_data,
        "Chassis": {"@odata.id": "/redfish/v1/Chassis"},
        "Managers": {"@odata.id": "/redfish/v1/Managers"},
        "SessionService": {"@odata.id": "/redfish/v1/SessionService"},
        "AccountService": {"@odata.id": "/redfish/v1/AccountService"},
    }
    await validate_keys(body)
    return web.json_response(body, dumps=dumps_bytestr)
