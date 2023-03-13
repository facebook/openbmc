# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# ResourceZone Template File

import copy
import strgen

_TEMPLATE = \
{
    "@Redfish.Copyright": "Copyright 2014-2019 DMTF. All rights reserved.",
    "@odata.context": "{rb}$metadata#Zone.Zone",
    "@odata.id": "{rb}CompositionService/ResourceZones/{id}",
    "@odata.type": "#Zone.v1_1_0.Zone",
    "Id": "{id}",
    "Name": "Resource Zone",
    "Status": {
            "State": "Enabled",
            "Health": "OK"
        },
    "Links": {
            "ResourceBlocks": [],
         },
    "@Redfish.CollectionCapabilities": {
            "@odata.type": "#CollectionCapabilities.v1_0_0.CollectionCapabilities",
            "Capabilities": [
                {
                    "CapabilitiesObject": {
                    "@odata.id": "{rb}Systems/Capabilities"
                    } ,
                    "UseCase": "ComputerSystemComposition",
                    "Links": {
                        "TargetCollection": {
                        "@odata.id": "{rb}Systems"
                        }
                    }
                }
            ]
         },

}


def get_ResourceZone_instance(wildcards):
    """
    Instantiate and format the template

    Arguments:
        wildcard - A dictionary of wildcards strings and their replacement values

    """
    c = copy.deepcopy(_TEMPLATE)

    c['@odata.context'] = c['@odata.context'].format(**wildcards)
    c['@odata.id'] = c['@odata.id'].format(**wildcards)
    c['Id'] = c['Id'].format(**wildcards)

    c['@Redfish.CollectionCapabilities']['Capabilities'][0]['CapabilitiesObject']['@odata.id'] = c['@Redfish.CollectionCapabilities']['Capabilities'][0]['CapabilitiesObject']['@odata.id'].format(**wildcards)
    c['@Redfish.CollectionCapabilities']['Capabilities'][0]['Links']['TargetCollection']['@odata.id'] = c['@Redfish.CollectionCapabilities']['Capabilities'][0]['Links']['TargetCollection']['@odata.id'].format(**wildcards)


    return c
