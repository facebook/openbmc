# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# EgSubResource Template File
#
# Replace the string "EgSubResource" with the proper resource
# names and adjust the TEMPLATE as necessary to match the schema
# for the resource.
#
# This file goes in the api_emulator/redfish/templates directory,
# and must be paired with an appropriate Resource API file in the
# api_emulator/redfish directory. The resource_manager.py
# file in the api_emulator directory can then be edited to use these
# files to make the resource dynamic.

import copy
import strgen

_TEMPLATE = \
    {
        "@odata.context": "{rb}$metadata#EgSubResource.EgSubResource",
        "@odata.type": "#EgSubResource.v1_0_0.EgSubResource",
        "@odata.id": "{rb}EgResources/{eg_id}/EgSubResources/{id}",
        "Id": "{id}",
        "Name": "Name of Example Subordinate Resource",
        "Description": "Example subordinate resource.",
        "Manufacturer": "Redfish Computers",
        "SerialNumber": "888el0456",
        "Version": "2.12",
        "PartNumber": "R889e-J23",
        "Status": {
            "State": "Enabled",
            "Health": "OK"
        }
    }

# Not used
'''
def get_EgSubResource_instance(rest_base, ident):
    """
    Formats the template

    Arguments:
        rest_base - Base URL for the RESTful interface
        indent    - ID of the resource
    """
    c = copy.deepcopy(_TEMPLATE)

    c['@odata.context'] = c['@odata.context'].format(rb=rest_base)
    c['@odata.id'] = c['@odata.id'].format(rb=rest_base, id=ident)
    c['Id'] = c['Id'].format(id=ident)

    c['SerialNumber'] = strgen.StringGenerator('[A-Z]{3}[0-9]{10}').render()

    return c
'''

def get_EgSubResource_instance(wildcards):
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
