# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# ResourceBlock Template File

import copy
import strgen

_TEMPLATE = \
{
    "@Redfish.Copyright": "Copyright 2014-2019 DMTF. All rights reserved.",
    "@odata.context": "{rb}$metadata#ResourceBlock.ResourceBlock",
    "@odata.id": "{rb}CompositionService/ResourceBlocks/{id}",
    "@odata.type": "#ResourceBlock.v1_0_0.ResourceBlock",
    "Id": "{id}",
    "Name": "Resource Block",
    "Status": {
            "State": "Enabled",
            "Health": "OK"
        },
    "CompositionStatus": {
            "Reserved": "false",
            "CompositionState": "Unused" # Unused or Composed
        },
    "Processors": [],
    "Memory": [],
    "Storage": [],
    "SimpleStorage": [],
    "EthernetInterfaces": [],
    "ComputerSystems": [],
    "Links": {
            "ComputerSystems": [],
            "Chassis": [],
            "Zones": [],
         },
}


def get_ResourceBlock_instance(wildcards):
    """
    Instantiate and format the template

    Arguments:
        wildcard - A dictionary of wildcards strings and their replacement values

    """
    c = copy.deepcopy(_TEMPLATE)

    c['@odata.context'] = c['@odata.context'].format(**wildcards)
    c['@odata.id'] = c['@odata.id'].format(**wildcards)
    c['Id'] = c['Id'].format(**wildcards)


    return c
