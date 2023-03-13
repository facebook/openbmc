# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Session Template File

import copy
import strgen

_TEMPLATE = \
{
    "@Redfish.Copyright":"Copyright 2014-2021 DMTF. All rights reserved.",
    "@odata.id": "{rb}SessionService/Sessions/{id}",
    "@odata.type": "#Session.v1_3_0.Session",
    "Id": "{id}",
    "Name": "Session {id}",
    "Description": "Manager User Session",
    "UserName": "Administrator",

}

def get_Session_instance(wildcards):
    """
    Instantiate and format the template

    Arguments:
        wildcard - A dictionary of wildcards strings and their replacement values

    """
    c = copy.deepcopy(_TEMPLATE)

    c['@odata.context'] = c['@odata.context'].format(**wildcards)
    c['@odata.id'] = c['@odata.id'].format(**wildcards)
    c['Id'] = c['Id'].format(**wildcards)
    c['Description'] = c['Description'].format(**wildcards)
    c['Name'] = c['Name'].format(**wildcards)
    c['UserName'] = c['UserName'].format(**wildcards)

    return c
