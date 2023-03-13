# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Chassis Template File

import copy
import strgen
from api_emulator.utils import replace_recurse

_TEMPLATE = \
    {
        "@odata.context": "{rb}$metadata#Chassis.Chassis",
        "@odata.id": "{rb}Chassis/{id}",
        "@odata.type": "#Chassis.v1_0_0.Chassis",
        "Id": "{id}",
        "Name": "Computer System Chassis",
        "ChassisType": "RackMount",
        "Manufacturer": "Redfish Computers",
        "Model": "3500RX",
        "SKU": "8675309",
        "SerialNumber": "437XR1138R2",
        "Version": "1.02",
        "PartNumber": "224071-J23",
        "AssetTag": "Chicago-45Z-2381",
        "Status": {
            "State": "Enabled",
            "Health": "OK"
        },
        "Thermal": {
            "@odata.id": "{rb}Chassis/{id}/Thermal"
        },
        "Power": {
            "@odata.id": "{rb}Chassis/{id}/Power"
        },
        "Links": {
            "ComputerSystems": [],
            "ResourceBlocks": [],
            "ManagedBy": [
                {
                    "@odata.id": "{rb}Managers/{linkMgr}"
                }
            ],
         },
    }


def get_Chassis_instance(wildcards):
    """
    Creates an instace of TEMPLATE and replace wildcards as specfied
    """
    c = copy.deepcopy(_TEMPLATE)
    compsys=[{"@odata.id": "{rb}Systems/{linkSystem}".format(rb=wildcards['rb'],linkSystem=x)}
             for x in wildcards['linkSystem']]
    rcblocks=[{"@odata.id":"{rb}CompositionService/ResourceBlocks/{linkRB}".format(rb=wildcards['rb'],linkRB=x)}
             for x in wildcards['linkResourceBlocks']]
    c['Links']['ComputerSystems']=compsys
    c['Links']['ResourceBlocks']=rcblocks
    replace_recurse(c, wildcards)
    return c
