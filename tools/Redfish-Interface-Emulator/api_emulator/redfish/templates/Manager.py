# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Manager Template File

import copy
import strgen

_TEMPLATE = \
{
    "@Redfish.Copyright": "Copyright 2014-2019 DMTF. All rights reserved.",
    "@odata.context": "{rb}$metadata#Manager.Manager",
    "@odata.id": "{rb}Managers/{id}",
    "@odata.type": "#Manager.v1_1_0.Manager",
    "Id": "{id}",
    "Name": "Manager",
    "ManagerType": "BMC",
    "Description": "BMC",
    "ServiceEntryPointUUID": "92384634-2938-2342-8820-489239905423",
    "UUID": "00000000-0000-0000-0000-000000000000",
    "Model": "Joo Janta 200",
    "DateTime": "2015-03-13T04:14:33+06:00",
    "DateTimeLocalOffset": "+06:00",
    "Status": {
        "State": "Enabled",
        "Health": "OK"
    },
    "GraphicalConsole": {
        "ServiceEnabled": True,
        "MaxConcurrentSessions": 2,
        "ConnectTypesSupported": [
            "KVMIP"
        ]
    },
    "SerialConsole": {
        "ServiceEnabled": True,
        "MaxConcurrentSessions": 1,
        "ConnectTypesSupported": [
            "Telnet",
            "SSH",
            "IPMI"
        ]
    },
    "CommandShell": {
        "ServiceEnabled": True,
        "MaxConcurrentSessions": 4,
        "ConnectTypesSupported": [
            "Telnet",
            "SSH"
        ]
    },
    "FirmwareVersion": "1.00",
    "NetworkProtocol": {
        "@odata.id": "{rb}Managers/{id}/NetworkProtocol"
    },
    "EthernetInterfaces": {
        "@odata.id": "{rb}Managers/{id}/EthernetInterfaces"
    },
    "SerialInterfaces": {
        "@odata.id": "{rb}Managers/{id}/SerialInterfaces"
    },
    "LogServices": {
        "@odata.id": "{rb}Managers/{id}/LogServices"
    },
    "VirtualMedia": {
        "@odata.id": "{rb}Managers/{id}/VM1"
    },
    "Links": {
        "ManagerForServers": [
            {
                "@odata.id": "{rb}Systems/{linkSystem}"
            }
        ],
        "ManagerForChassis": [
            {
                "@odata.id": "{rb}Chassis/{linkChassis}"
            }
        ],
        "ManagerInChassis": {
            "@odata.id": "{rb}Chassis/{linkInChassis}"
        }
    },
    "Actions": {
        "#Manager.Reset": {
            "target": "{rb}Managers/{id}/Actions/Manager.Reset",
            "ResetType@Redfish.AllowableValues": [
                "ForceRestart",
                "GracefulRestart"
            ]
        }
    }
}

def get_Manager_instance(wildcards):
    """
    Instantiate and format the template

    Arguments:
        wildcard - A dictionary of wildcards strings and their repalcement values

    """
    c = copy.deepcopy(_TEMPLATE)

    c['@odata.context'] = c['@odata.context'].format(**wildcards)
    c['@odata.id'] = c['@odata.id'].format(**wildcards)
    c['Id'] = c['Id'].format(**wildcards)

    c['NetworkProtocol']['@odata.id'] = c['NetworkProtocol']['@odata.id'].format(**wildcards)
    c['EthernetInterfaces']['@odata.id'] = c['EthernetInterfaces']['@odata.id'].format(**wildcards)
    c['SerialInterfaces']['@odata.id'] = c['SerialInterfaces']['@odata.id'].format(**wildcards)
    c['LogServices']['@odata.id'] = c['LogServices']['@odata.id'].format(**wildcards)
    c['VirtualMedia']['@odata.id'] = c['VirtualMedia']['@odata.id'].format(**wildcards)

    systems=wildcards['linkSystem']
    if type(systems) is list:
        mfs=[{'@odata.id':c['Links']['ManagerForServers'][0]['@odata.id'].format(rb=wildcards['rb'], linkSystem=x)}
             for x in systems]
    else:
        mfs=[{'@odata.id':c['Links']['ManagerForServers'][0]['@odata.id'].format(**wildcards)}]

    c['Links']['ManagerForServers'] = mfs
    c['Links']['ManagerForChassis'][0]['@odata.id'] = c['Links']['ManagerForChassis'][0]['@odata.id'].format(**wildcards)

    c['Links']['ManagerInChassis']['@odata.id'] = c['Links']['ManagerInChassis']['@odata.id'].format(**wildcards)

    c['Actions']['#Manager.Reset']['target'] = c['Actions']['#Manager.Reset']['target'].format(**wildcards)

    return c
