import rest_pal_legacy
from aiohttp import web
from common_utils import dumps_bytestr
from redfish_base import validate_keys


async def get_redfish(request: str) -> web.Response:
    body = {"v1": "/redfish/v1/"}
    return web.json_response(body, dumps=dumps_bytestr)


async def get_service_root(request: str) -> web.Response:
    product_name = str(rest_pal_legacy.pal_get_platform_name())
    uuid_data = str(rest_pal_legacy.pal_get_uuid())
    body = {
        "@odata.context": "/redfish/v1/$metadata#ServiceRoot.ServiceRoot",
        "@odata.id": "/redfish/v1/",
        "@odata.type": "#ServiceRoot.v1_9_0.ServiceRoot",
        "Id": "RootService",
        "Name": "Root Service",
        "Product": product_name,
        "RedfishVersion": "1.6.0",
        "UUID": uuid_data,
        "SessionService": {"@odata.id": "/redfish/v1/SessionService"},
        "Chassis": {"@odata.id": "/redfish/v1/Chassis"},
        "Managers": {"@odata.id": "/redfish/v1/Managers"},
        "AccountService": {"@odata.id": "/redfish/v1/AccountService"},
        "Links": {"Sessions": {"@odata.id": "/redfish/v1/SessionService/Sessions"}},
    }
    await validate_keys(body)
    return web.json_response(body, dumps=dumps_bytestr)


async def get_odata(request: str) -> web.Response:
    body = {
        "@odata.context": "/redfish/v1/$metadata",
        "@odata.id": "/redfish/v1/odata",
        "@odata.type": "OData",
        "value": [
            {"name": "Service", "kind": "Singleton", "url": "/redfish/v1/"},
            {"name": "Chassis", "kind": "Singleton", "url": "/redfish/v1/Chassis"},
        ],
    }
    await validate_keys(body)
    return web.json_response(body, dumps=dumps_bytestr)


async def get_metadata(request: str) -> web.Response:
    body = """
<?xml version="1.0" encoding="UTF-8"?>
<edmx:Edmx xmlns:edmx="http://docs.oasis-open.org/odata/ns/edmx" Version="4.0">
    <edmx:Reference Uri="schemas/v1/ServiceRoot_v1.xml">
        <edmx:Include Namespace="ServiceRoot"/>
        <edmx:Include Namespace="ServiceRoot.v1_0_0"/>
    </edmx:Reference>
    <edmx:Reference Uri="schemas/v1/ChassisCollection_v1.xml">
        <edmx:Include Namespace="ChassisCollection"/>
    </edmx:Reference>
    <edmx:Reference Uri="schemas/v1/Chassis_v1.xml">
        <edmx:Include Namespace="Chassis"/>
        <edmx:Include Namespace="Chassis.v1_5_0"/>
    </edmx:Reference>
    <edmx:Reference Uri="schemas/v1/Thermal_v1.xml">
        <edmx:Include Namespace="Thermal"/>
        <edmx:Include Namespace="Thermal.v1_3_0"/>
        <edmx:Include Namespace="Thermal.v1_7_0"/>
    </edmx:Reference>
    <edmx:DataServices>
        <Schema xmlns="http://docs.oasis-open.org/odata/ns/edm" Namespace="Service">
        <EntityContainer Name="Service" Extends="ServiceRoot.v1_0_0.ServiceContainer"/>
        </Schema>
    </edmx:DataServices>
</edmx:Edmx>
"""
    return web.Response(text=body, content_type="application/xml")
