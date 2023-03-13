# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# EgResource Template File
#
# Replace the strings "EgResource" and "EgSubResource" with
# the proper resource names and adjust the TEMPLATE as necessary to
# match the schema for the resource.
#
# This file goes in the api_emulator/redfish/templates directory,
# and must be paired with an appropriate Resource API file in the
# api_emulator/redfish directory. The resource_manager.py
# file in the api_emulator directory can then be edited to use these
# files to make the resource dynamic.

import copy
import strgen
from api_emulator.utils import replace_recurse

_TEMPLATE = \
    {
        "@odata.context": "{rb}$metadata#EgResource.EgResource",
        "@odata.type": "#EgResource.v1_0_0.EgResource",
        "@odata.id": "{rb}EgResources/{id}",
        "Id": "{id}",
        "Name": "Name of Example Resource",
        "Description": "Example resource.",
        "Manufacturer": "Redfish Computers",
        "SerialNumber": "437XR1138R2",
        "Version": "1.02",
        "PartNumber": "224071-J23",
        "Status": {
            "State": "Enabled",
            "Health": "OK"
        },
        "SubResources": {
            "@odata.id": "{rb}EgResources/{id}/EgSubResources"
        }
    }

def get_EgResource_instance(wildcards):
    """
    Creates an instance of TEMPLATE and replaces wildcards as specfied.
    Also sets any unique values.

    Arguments:
        wildcard - A dictionary of wildcards strings and their replacement values
    """

    c = copy.deepcopy(_TEMPLATE)
    replace_recurse(c, wildcards)
    # print ("fini c: ", c)

    c['SerialNumber'] = strgen.StringGenerator('[A-Z]{3}[0-9]{10}').render()

    return c
