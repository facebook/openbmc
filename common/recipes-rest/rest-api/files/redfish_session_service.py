from aiohttp import web
from common_utils import dumps_bytestr
from redfish_base import validate_keys


async def get_session_service(request: web.Request) -> web.Response:
    body = {
        "@odata.type": "#SessionService.v1_1_8.SessionService",
        "Id": "SessionService",
        "Name": "Session Service",
        "Description": "Session Service",
        "Status": {"State": "Enabled", "Health": "OK"},
        "ServiceEnabled": True,
        "SessionTimeout": 30,
        "Sessions": {
            "@odata.id": "/redfish/v1/SessionService/Sessions",
        },
        "@odata.id": "/redfish/v1/SessionService",
    }
    await validate_keys(body)
    return web.json_response(body, dumps=dumps_bytestr)


async def get_session(request: web.Request) -> web.Response:
    headers = {"Link": "</redfish/v1/schemas/SessionCollection.json>; rel=describedby"}
    body = {
        "@odata.type": "#SessionCollection.SessionCollection",
        "Name": "Session Collection",
        "Members@odata.count": 0,
        "Members": [],
        "@odata.id": "/redfish/v1/SessionService/Sessions",
    }
    await validate_keys(body)
    return web.json_response(body, headers=headers, dumps=dumps_bytestr)
