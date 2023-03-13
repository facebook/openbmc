#!/usr/bin/env python
# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# ComputerSystem.py

import copy
import strgen
from api_emulator.utils import replace_recurse

_TEMPLATE = \
{
    "@Redfish.Copyright": "Copyright 2014-2019 DMTF. All rights reserved.",
    "@odata.context": "/redfish/v1/$metadata#ComputerSystem.ComputerSystem",
    "@odata.id": "/redfish/v1/Systems/1",
    "@odata.type": "#ComputerSystem.v1_3_0.ComputerSystem",
    "Id": "1",
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
        "@odata.id": "/redfish/v1/Systems/1/Processors"
    },
    "Memory": {
        "@odata.id": "/redfish/v1/Systems/1/Memory"
    },	
    "EthernetInterfaces": {
        "@odata.id": "/redfish/v1/Systems/1/EthernetInterfaces"
    },
    "NetworkInterfaces": {
        "@odata.id": "/redfish/v1/Systems/1/NetworkInterfaces"
    },
    "SimpleStorage": {
        "@odata.id": "/redfish/v1/Systems/1/SimpleStorage"
    },
    "LogServices": {
        "@odata.id": "/redfish/v1/Systems/1/LogServices"
    },
    "SecureBoot": {
        "@odata.id": "/redfish/v1/Systems/1/SecureBoot"
    },
    "Bios": {
        "@odata.id": "/redfish/v1/Systems/1/Bios"
    },
    "PCIeDevices": [
        {"@odata.id": "/redfish/v1/Chassis/1/PCIeDevices/NIC"}
    ],
    "PCIeFunctions": [
        {"@odata.id": "/redfish/v1/Chassis/1/PCIeDevices/NIC/Functions/1"},
        {"@odata.id": "/redfish/v1/Chassis/1/PCIeDevices/NIC/Functions/2"}
    ],
    "Links": {
        "Chassis": [
            {
                "@odata.id": "/redfish/v1/Chassis/1"
            }
        ],
        "ManagedBy": [
            {
                "@odata.id": "/redfish/v1/Managers/1"
            }
        ],
        "Endpoints": [
            {
                "@odata.id": "/redfish/v1/Fabrics/PCIe/Endpoints/HostRootComplex1"
            }
        ],
        "Oem": {}
    },
    "Actions": {
        "#ComputerSystem.Reset": {
            "target": "/redfish/v1/Systems/1/Actions/ComputerSystem.Reset",
	    "@Redfish.ActionInfo": "/redfish/v1/Systems/1/ResetActionInfo"
        },
        "Oem": {
            "#Contoso.Reset": {
                "target": "/redfish/v1/Systems/1/OEM/Contoso/Actions/Contoso.Reset",
		"@Redfish.ActionInfo": "/redfish/v1/Systems/1/ContosoResetActionInfo"
            }
        }
    },
    "Oem": {
        "Contoso": {
            "@odata.type": "#Contoso.ComputerSystem",
            "ProductionLocation": {
                "FacilityName": "PacWest Production Facility",
                "Country": "USA"
            }
        },
        "Chipwise": {
            "@odata.type": "#Chipwise.ComputerSystem",
            "Style": "Executive"
        }
    }
}


def get_ComputerSystem_instance(wildcards):
    c = copy.deepcopy(_TEMPLATE)
    replace_recurse(c, wildcards)
    return c
