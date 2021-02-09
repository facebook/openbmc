import uuid
from node import node
from pal import pal_get_uuid, pal_get_platform_name

def get_redfish():
    body = {}
    try:
       body = {
           "v1": "/redfish/v1"
       }
    except Exception as error:
        print(error)
    return node(body)


def get_service_root():
    body = {}
    try:
        #uuid_data = str(uuid.uuid1())
        name = pal_get_platform_name()
        uuid_data = pal_get_uuid()
        body = {
            "@odata.context": "/redfish/v1/$metadata#ServiceRoot.ServiceRoot",
            "@odata.id": "/redfish/v1/",
            "@odata.type": "#ServiceRoot.v1_3_0.ServiceRoot",
            "Id": "RootService",
            "Name": "Root Service",
            "Product": str(name),
            "RedfishVersion": "1.0.0",
            "UUID": str(uuid_data),
            "Chassis": {
                "@odata.id": "/redfish/v1/Chassis"
            },
            "Managers": {
                "@odata.id": "/redfish/v1/Managers"
            },
            "SessionService": {
                "@odata.id": "/redfish/v1/SessionService"
            },
            "AccountService": {
                "@odata.id": "/redfish/v1/AccountService"
            }
        }
    except Exception as error:
        print(error)

    return node(body)
