# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Example Resoruce Template
import copy
import strgen

_TEMPLATE = \
{
    "@Redfish.Copyright": "Copyright 2014-2019 DMTF. All rights reserved.",
    "@odata.context": "{rb}$metadata#ComputerSystem.ComputerSystem",
    "@odata.id": "{rb}Systems/{id}",
    "@odata.type": "#ComputerSystem.v1_3_0.ComputerSystem",
    "Id": "{id}",
    "Name": "My Computer System",
    "SystemType": "Physical",
    "AssetTag": "free form asset tag",
    "Manufacturer": "Manufacturer Name",
    "Model": "Model Name",
    "SKU": "",
    "SerialNumber": "2M220100SL",
    "PartNumber": "",
    "Description": "Description of server",
    "UUID": "00000000-0000-0000-0000-000000000000",
    "HostName": "web-srv344",
    "Status": {
        "State": "Enabled",
        "Health": "OK",
        "HealthRollup": "OK"
    },
    "IndicatorLED": "Off",
    "PowerState": "On",
    "Boot": {
        "BootSourceOverrideEnabled": "Once",
        "BootSourceOverrideMode": "UEFI",
        "BootSourceOverrideTarget": "Pxe",
        "BootSourceOverrideTarget@Redfish.AllowableValues": [
            "None",
            "Pxe",
            "Floppy",
            "Cd",
            "Usb",
            "Hdd",
            "BiosSetup",
            "Utilities",
            "Diags",
            "UefiTarget",
            "SDCard",
            "UefiHttp"
        ],
        "UefiTargetBootSourceOverride": "uefi device path"
    },
    "BiosVersion": "P79 v1.00 (09/20/2013)",
    "ProcessorSummary": {
        "Count": 8,
        "Model": "Multi-Core Intel(R) Xeon(R) processor 7xxx Series",
        "Status": {
            "State": "Enabled",
            "Health": "OK",
            "HealthRollup": "OK"
        }
    },
    "MemorySummary": {
        "TotalSystemMemoryGiB": 16,
        "MemoryMirroring" : "System",
        "Status": {
            "State": "Enabled",
            "Health": "OK",
            "HealthRollup": "OK"
        }
    },
    "TrustedModules": [
        {
          "Status": {
            "State": "Enabled",
            "Health": "OK"
           },
           "ModuleType":"TPM2_0",
           "FirmwareVersion": "3.1",
           "FirmwareVersion2": "1",
           "InterfaceTypeSelection": "None"
        }
    ],
    "Processors": {
        "@odata.id": "{rb}Systems/{id}/Processors"
    },
    "Memory": {
        "@odata.id": "{rb}Systems/{id}/Memory"
    },
    "EthernetInterfaces": {
        "@odata.id": "{rb}Systems/{id}/EthernetInterfaces"
    },
    "SimpleStorage": {
        "@odata.id": "{rb}Systems/{id}/SimpleStorage"
    },
    "Links": {
        "Chassis": [
            {
                "@odata.id": "{rb}Chassis/{linkChassis}"
            }
        ],
        "ManagedBy": [
            {
                "@odata.id": "{rb}Managers/{linkMgr}"
            }
        ],
        "Oem": {}
    },
    "Actions": {
        "#ComputerSystem.Reset": {
            "target": "{rb}Systems/{id}/Actions/ComputerSystem.Reset", "@Redfish.ActionInfo": "{rb}Systems/{id}/ResetActionInfo"
        },
    }
}


def get_ComputerSystem_instance(wildcards):
    """
    Instantiate and format the template

    Arguments:
        wildcard - A dictionary of wildcards strings and their repalcement values

    """
    c = copy.deepcopy(_TEMPLATE)

    c['@odata.context'] = c['@odata.context'].format(**wildcards)
    c['@odata.id'] = c['@odata.id'].format(**wildcards)
    c['Id'] = c['Id'].format(**wildcards)

    c['Processors']['@odata.id'] = c['Processors']['@odata.id'].format(**wildcards)
    c['Memory']['@odata.id'] = c['Memory']['@odata.id'].format(**wildcards)
    c['EthernetInterfaces']['@odata.id'] = c['EthernetInterfaces']['@odata.id'].format(**wildcards)
    c['SimpleStorage']['@odata.id'] = c['SimpleStorage']['@odata.id'].format(**wildcards)

    chassis=[{'@odata.id':"{rb}Chassis/{linkChassis}".format(rb=wildcards['rb'],linkChassis=x)}
             for x in wildcards['linkChassis']]
    c['Links']['Chassis'] = chassis
    c['Links']['ManagedBy'][0]['@odata.id'] = c['Links']['ManagedBy'][0]['@odata.id'].format(**wildcards)

    c['Actions']['#ComputerSystem.Reset']['target'] = c['Actions']['#ComputerSystem.Reset']['target'].format(**wildcards)
    c['Actions']['#ComputerSystem.Reset']['@Redfish.ActionInfo'] = c['Actions']['#ComputerSystem.Reset']['@Redfish.ActionInfo'].format(**wildcards)

    return c
