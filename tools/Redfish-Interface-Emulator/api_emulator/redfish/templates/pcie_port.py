# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# PCIePort Template File

import copy

PCIePort_TEMPLATE={
    "@odata.context": "{rb}$metadata#PCIeSwitches/Members/{sw_id}/Ports/Members$entity",
    "@odata.type": "PCIePort.v1_0_0.PCIePort",
    "@odata.id": "{rb}PCIeSwitches/{sw_id}/Ports/{id}",

    "Id": "{id}",
    "Name": "PCIe Port",
    "Description": "PCIe port",
    "PortType": "Upstream",
    "PhysicalPort": "TRUE",

    "PortId": "2",
    "SpeedGBps": 4,
    "Width": 4,
    "MaxSpeedGBps": 8,
    "MaxWidth": 8,

    "OperationalState": "Up",
    "AdministrativeState": "Up",
    "Status": {
        "State": "Enabled",
        "Health": "OK"
    },

    "Actions": {
        "#PCIePort.SetState": {
            "target": "{rb}PCIeSwitches/{sw_id}/Ports/{id}/Actions/Port.SetState",
            "SetStateType@Redfish.AllowableValues": [
                "Up",
                "Down",
            ]
    },
}
    }


def get_PCIePort_instance(rest_base,sw_id,ident):

    c = copy.deepcopy(PCIePort_TEMPLATE)

    c['@odata.context']=c['@odata.context'].format(rb=rest_base,sw_id=sw_id)
    c['@odata.id']=c['@odata.id'].format(rb=rest_base,sw_id=sw_id,id=ident)
    c['Id'] = c['Id'].format(id=ident)
    c['Actions']['#PCIePort.SetState']['target']=c['Actions']['#PCIePort.SetState']['target'].format(rb=rest_base,sw_id=sw_id,id=ident)

    return c
