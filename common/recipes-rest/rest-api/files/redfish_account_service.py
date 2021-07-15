from aiohttp import web
from common_utils import dumps_bytestr
from redfish_base import validate_keys


async def get_account_service(request: str) -> web.Response:
    body = {
        "@odata.context": "/redfish/v1/$metadata#AccountService.AccountService",
        "@odata.id": "/redfish/v1/AccountService",
        "@odata.type": "#AccountService.v1_0_0.AccountService",
        "Id": "AccountService",
        "Name": "Account Service",
        "Description": "Account Service",
        "Accounts": {"@odata.id": "/redfish/v1/AccountService/Accounts"},
    }
    await validate_keys(body)
    return web.json_response(body, dumps=dumps_bytestr)
