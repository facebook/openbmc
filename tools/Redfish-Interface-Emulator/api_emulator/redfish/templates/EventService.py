# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# EventService Template File

import copy
import strgen

_TEMPLATE = \
{
    "@Redfish.Copyright": "Copyright 2014-2019 DMTF. All rights reserved.",
    "@odata.context": "{rb}$metadata#EventService.EventService",
    "@odata.type": "#EventService.v1_0_0.EventService",
    "@odata.id": "{rb}EventService",
    "Id": "{id}",
    "Name": "Event Service",
    "Status": {
        "State": "Enabled",
        "Health": "OK"
    },
    "ServiceEnabled": True,
    "DeliveryRetryAttempts": 3,
    "DeliveryRetryIntervalSeconds": 60,
    "EventTypesForSubscription": [
        "StatusChange",
        "ResourceUpdated",
        "ResourceAdded",
        "ResourceRemoved",
        "Alert"
    ],
    "Subscriptions": {
        "@odata.id": "{rb}EventService/Subscriptions"
    },
    "Actions": {
        "#EventService.SubmitTestEvent": {
            "target": "{rb}EventService/Actions/EventService.SubmitTestEvent",
            "EventType@Redfish.AllowableValues": [
                "StatusChange",
                "ResourceUpdated",
                "ResourceAdded",
                "ResourceRemoved",
                "Alert"
            ]
        }
    }
}


def get_EventService_instance(wildcards):
    """
    Instantiates and formats the template

    Arguments:
        wildcard - A dictionary of wildcards strings and their replacement values
    """
    c = copy.deepcopy(_TEMPLATE)

    c['@odata.context'] = c['@odata.context'].format(**wildcards)
    c['@odata.id'] = c['@odata.id'].format(**wildcards)
    c['Id'] = c['Id'].format(**wildcards)
    c['Subscriptions']['@odata.id'] = c['Subscriptions']['@odata.id'].format(**wildcards)
    c['Actions']['#EventService.SubmitTestEvent']['target'] = c['Actions']['#EventService.SubmitTestEvent']['target'].format(**wildcards)

    return c
