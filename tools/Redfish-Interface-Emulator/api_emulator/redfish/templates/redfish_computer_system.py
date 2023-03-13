# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Redfish template 

REDFISH_TEMPLATE  = {
    "@odata.context": "{rest_base}$metadata#Systems/cs_puid",
    "@odata.id": "{rest_base}Systems/{cs_puid}",
    "@odata.type": '#ComputerSystem.1.0.0.ComputerSystem',
    "Id": None,
    "Name": "WebFrontEnd483",

    "SystemType": "Virtual",
    "AssetTag": "Chicago-45Z-2381",
    "Manufacturer": "Redfish Computers",
    "Model": "3500RX",
    "SKU": "8675309",
    "SerialNumber": None,

    "PartNumber": "224071-J23",
    "Description": "Web Front End node",

    "UUID": None,
    "HostName":"web483",
    "Status": {
        "State": "Enabled",
        "Health": "OK",
        "HealthRollUp": "OK"
    },

    "IndicatorLED": "Off",
    "PowerState": "On",
    "Boot": {
        "BootSourceOverrideEnabled": "Once",
        "BootSourceOverrideTarget": "Pxe",
        "BootSourceOverrideTarget@DMTF.AllowableValues": [
            "None",
            "Pxe",
            "Floppy",
            "Cd",
            "Usb",
            "Hdd",
            "BiosSetup",
            "Utilities",
            "Diags",
            "UefiTarget"
        ],
        "UefiTargetBootSourceOverride": "/0x31/0x33/0x01/0x01"
    },
    "Oem":{},

    "BiosVersion": "P79 v1.00 (09/20/2013)",
    "Processors": {
        "Count": 8,
        "Model": "Multi-Core Intel(R) Xeon(R) processor 7xxx Series",
        "Status": {
            "State": "Enabled",
            "Health": "OK",
            "HealthRollUp": "OK"
        }
    },
    "Memory": {
        "TotalSystemMemoryGB": 16,
        "Status": {
            "State": "Enabled",
            "Health": "OK",
            "HealthRollUp": "OK"
        }
    },
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
        "Processors": {
            "@odata.id": "/redfish/v1/Systems/{cs_puid}/Processors"
        },
        "EthernetInterfaces": {
            "@odata.id": "/redfish/v1/Systems/{cs_puid}/EthernetInterfaces"
        },
        "SimpleStorage": {
            "@odata.id": "/redfish/v1/Systems/{cs_puid}/SimpleStorage"
        },
        "LogService": {
            "@odata.id": "/redfish/v1/Systems/1/Logs"
        }
    },
    "Actions": {
        "#ComputerSystem.Reset": {
            "target": "/redfish/v1/Systems/{cs_puid}/Actions/ComputerSystem.Reset",
            "ResetType@DMTF.AllowableValues": [
                "On",
                "ForceOff",
                "GracefulRestart",
                "ForceRestart",
                "Nmi",
                "GracefulRestart",
                "ForceOn",
                "PushPowerButton"
            ]
        },
        "Oem": {
            "http://Contoso.com/Schema/CustomTypes#Contoso.Reset": {
                "target": "/redfish/v1/Systems/1/OEM/Contoso/Actions/Contoso.Reset"
            }
        }
    },

        "Oem": {
            "Contoso": {
            "@odata.type": "http://Contoso.com/Schema#Contoso.ComputerSystem",
            "ProductionLocation": {
                "FacilityName": "PacWest Production Facility",
                "Country": "USA"
            }
        },
       "Chipwise": {
            "@odata.type": "http://Chipwise.com/Schema#Chipwise.ComputerSystem",
            "Style": "Executive"
        }
    }
}
