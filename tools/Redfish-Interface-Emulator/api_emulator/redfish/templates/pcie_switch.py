# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# PCIeSwitch Template File

import copy

PCIeSwitch_TEMPLATE={
    "@odata.type": "#PCIeSwitch.v1_0_0.PCIeSwitch",
    "@odata.context": "{rb}$metadata#PCIeSwitches/Members/$entity",
    "@odata.id": "{rb}PCIeSwitches/{id}",

    "Name": "PCIe Switch",
    "Id": "{id}",

    "TotalLanes": 196,
    "MaxLaneBandwidthGBps": 1,

    "Manufacturer": "Intel Corporation",
    "Model": "Intel PCIe Switch",
    "SerialNumber": "2M220100SL",
    "PartNumber": "",
    "Status": {
        "State": "Enabled",
        "Health": "OK"
    },

    "Ports": {
        "@odata.id": "{rb}PCIeSwitches/{id}/Ports"
    },
    "Devices": {
        "@odata.id": "{rb}PCIeSwitches/{id}/Devices"
    },
    "FunctionMaps": {
        "@odata.id": "{rb}PCIeSwitches/{id}/Zones"
    },
    "Links": {
        "Chassis": [
            {
                "@odata.id": "{rb}Chassis/1"
            }
        ],
        "ManagedBy": [
            {
                "@odata.id": "{rb}Managers/Manager_1"
            }
        ]
    },
    "Actions": {
        "#PCIeSwitch.Reset": {
            "target": "{rb}PCIeSwitches/{id}/Actions/PCIeSwitch.Reset",
            "SwitchResetType@Redfish.AllowableValues": [
                "On",
                "Off",
                "Restart",
            ]
        }
    }
}

def get_PCIeSwitch_instance(rest_base,ident):

    # Perform deepcopy for dictionaries
    c=copy.deepcopy(PCIeSwitch_TEMPLATE)

    # Replace variables with correct values for this instance
    c['@odata.context'] = c['@odata.context'].format(rb=rest_base)
    c['@odata.id'] = c['@odata.id'].format(rb=rest_base, id=ident)
    c['Id'] = c['Id'].format(id=ident)
    c['Devices']['@odata.id']=c['Devices']['@odata.id'].format(rb=rest_base, id=ident)
    c['Ports']['@odata.id']=c['Ports']['@odata.id'].format(rb=rest_base, id=ident)
    c['FunctionMaps']['@odata.id']=c['FunctionMaps']['@odata.id'].format(rb=rest_base,id=ident)
    c['Links']['Chassis'][0]['@odata.id']=c['Links']['Chassis'][0]['@odata.id'].format(rb=rest_base)
    c['Links']['ManagedBy'][0]['@odata.id']=c['Links']['ManagedBy'][0]['@odata.id'].format(rb=rest_base)
    c['Actions']['#PCIeSwitch.Reset']['target']= c['Actions']['#PCIeSwitch.Reset']['target'].format(rb=rest_base,id=ident)

    return c
