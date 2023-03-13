# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# SessionService Template File

import copy
import strgen

_TEMPLATE = \
{
    "@Redfish.Copyright": "Copyright 2014-2021 DMTF. All rights reserved.",
    "@odata.context": "{rb}$metadata#SessionService.SessionService",
    "@odata.type": "#SessionService.v1_1_8.SessionService",
    "@odata.id": "{rb}SessionService",
    "Id": "{id}",
    "Name": "Session Service",
    "Description": "Session Service",
    "Status": {
        "State": "Enabled",
        "Health": "OK"
    },
    "ServiceEnabled": "true",
    "SessionTimeout": 30,
    "Sessions": {
        "@odata.id": "/redfish/v1/SessionService/Sessions"
    }
}


def get_SessionService_instance(wildcards):
    """
    Instantiates and formats the template

    Arguments:
        wildcard - A dictionary of wildcards strings and their replacement values
    """
    c = copy.deepcopy(_TEMPLATE)

    c['@odata.context'] = c['@odata.context'].format(**wildcards)
    c['@odata.id'] = c['@odata.id'].format(**wildcards)
    c['Id'] = c['Id'].format(**wildcards)
    c['Name'] = c['Name'].format(**wildcards)
    c['Sessions']['@odata.id'] = c['Sessions']['@odata.id'].format(**wildcards)

    return c
