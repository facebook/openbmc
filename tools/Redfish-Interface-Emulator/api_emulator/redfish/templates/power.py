# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Power Template File

import copy

_TEMPLATE = \
    {
    "@odata.context": "{rb}$metadata#Chassis/Links/Members/{ch_id}/Links/Power/$entity",
    "@odata.id": "{rb}Chassis/{ch_id}/Power",
    "@odata.type": "#Power.v1_0_0.PowerMetrics",
    "Id": "PowerMetrics",
    "Name": "Power",
    "PowerControl": [
        {
            "@odata.id": "{rb}Chassis/{ch_id}/PowerMetrics#/PowerControl/0",
            "MemberId": "0",
            "Name": "System Power Control",
            "PowerConsumedWatts": 8000,
            "PowerRequestedWatts": 8500,
            "PowerAvailableWatts": 8500,
            "PowerCapacityWatts": 10000,
            "PowerAllocatedWatts": 8500,
            "PowerMetrics": {
                "IntervalInMin": 30,
                "MinConsumedWatts": 7500,
                "MaxConsumedWatts": 8200,
                "AverageConsumedWatts": 8000
            },
            "PowerLimit": {
                "LimitInWatts": 9000,
                "LimitException": "LogEventOnly",
                "CorrectionInMs": 42
            },
            "RelatedItem": [
                {"@odata.id": "{rb}Systems/{ch_id}"},
                {"@odata.id": "{rb}Chassis/{ch_id}"},
            ],
            "Status": {
                "State": "Enabled",
                "Health": "OK"
            },
        }
    ],
    "Voltages": [
        {
            "@odata.id": "{rb}Chassis/{ch_id}/PowerMetrics#/Voltages/0",
            "MemberId": "0",
            "Name": "VRM1 Voltage",
            "SensorNumber": 11,
            "Status": {
                "State": "Enabled",
                "Health": "OK"
            },
            "ReadingVolts": 12,
            "UpperThresholdNonCritical": 12.5,
            "UpperThresholdCritical": 13,
            "UpperThresholdFatal": 15,
            "LowerThresholdNonCritical": 11.5,
            "LowerThresholdCritical": 11,
            "LowerThresholdFatal": 10,
            "MinReadingRange": 0,
            "MaxReadingRange": 20,
            "PhysicalContext": "VoltageRegulator",
            "RelatedItem": [
                {"@odata.id": "{rb}Systems/{ch_id}" },
                {"@odata.id": "{rb}Chassis/{ch_id}" }
            ]
        },
        {
            "@odata.id": "{rb}Chassis/{ch_id}/PowerMetrics#/Voltages/1",
            "MemberId": "1",
            "Name": "VRM2 Voltage",
            "SensorNumber": 12,
            "Status": {
                "State": "Enabled",
                "Health": "OK"
            },
            "ReadingVolts": 5,
            "UpperThresholdNonCritical": 5.5,
            "UpperThresholdCritical": 7,
            "LowerThresholdNonCritical": 4.75,
            "LowerThresholdCritical": 4.5,
            "MinReadingRange": 0,
            "MaxReadingRange": 20,
            "PhysicalContext": "VoltageRegulator",
            "RelatedItem": [
                {"@odata.id": "{rb}Systems/{ch_id}" },
                {"@odata.id": "{rb}Chassis/{ch_id}" }
            ]
        }
    ],

    "PowerSupplies": [
        {
            "@odata.id": "{rb}Chassis/{ch_id}/Power#/PowerSupplies/0",
            "MemberId": "0",
            "Name": "Power Supply Bay 1",
            "Status": {
                    "State": "Enabled",
                    "Health": "Warning"
            },
            "PowerSupplyType": "DC",
            "LineInputVoltageType": "DCNeg48V",
            "LineInputVoltage": -48,
            "PowerCapacityWatts": 400,
            "LastPowerOutputWatts": 192,
            "Model": "499253-B21",
            "FirmwareVersion": "1.00",
            "SerialNumber": "1z0000001",
            "PartNumber": "1z0000001A3a",
            "SparePartNumber": "0000001A3a",
            "RelatedItem": [
                { "@odata.id": "{rb}Chassis/{ch_id}" }
            ],
        },
        {
            "@odata.id": "{rb}Chassis/{ch_id}/Power#/PowerSupplies/1",
            "MemberId": "1",
            "Name": "Power Supply Bay 2",
            "Status": {
                "State": "Disabled",
                "Health": "Warning"
            },
            "PowerSupplyType": "AC",
            "LineInputVoltageType": "ACMidLine",
            "LineInputVoltage": 220,
            "PowerCapacityWatts": 400,
            "LastPowerOutputWatts": 190,
            "Model": "499253-B21",
            "FirmwareVersion": "1.00",
            "SerialNumber": "1z0000001",
            "PartNumber": "1z0000001A3a",
            "SparePartNumber": "0000001A3a",
            "RelatedItem": [
                { "@odata.id": "{rb}Chassis/{ch_id}" }
            ],
        },
        {
            "@odata.id": "{rb}Chassis/{ch_id}/Power#/PowerSupplies/2",
            "MemberId": "2",
            "Name": "Power Supply Bay 3",
            "Status": {
                "State": "Absent"
            }
        }
    ]
}


def get_power_instance(wildcards):
    """
    Returns a formatted template

    Arguments:
        rest_base - Base URL of the RESTful interface
        ident     - Identifier of the chassis
    """
    c = copy.deepcopy(_TEMPLATE)

    c['@odata.context'] = c['@odata.context'].format(**wildcards)
    c['@odata.id'] = c['@odata.id'].format(**wildcards)
    c['PowerControl'][0]['@odata.id']=c['PowerControl'][0]['@odata.id'].format(**wildcards)
    c['PowerControl'][0]['RelatedItem'][0]['@odata.id']=c['PowerControl'][0]['RelatedItem'][0]['@odata.id'].format(**wildcards)
    c['PowerControl'][0]['RelatedItem'][1]['@odata.id']=c['PowerControl'][0]['RelatedItem'][1]['@odata.id'].format(**wildcards)
    c['Voltages'][0]['@odata.id']=c['Voltages'][0]['@odata.id'].format(**wildcards)
    c['Voltages'][0]['RelatedItem'][0]['@odata.id']=c['Voltages'][0]['RelatedItem'][0]['@odata.id'].format(**wildcards)
    c['Voltages'][0]['RelatedItem'][1]['@odata.id']=c['Voltages'][0]['RelatedItem'][1]['@odata.id'].format(**wildcards)
    c['Voltages'][1]['@odata.id']=c['Voltages'][1]['@odata.id'].format(**wildcards)
    c['Voltages'][1]['RelatedItem'][0]['@odata.id']=c['Voltages'][1]['RelatedItem'][0]['@odata.id'].format(**wildcards)
    c['Voltages'][1]['RelatedItem'][1]['@odata.id']=c['Voltages'][1]['RelatedItem'][1]['@odata.id'].format(**wildcards)
    c['PowerSupplies'][0]['@odata.id']=c['PowerSupplies'][0]['@odata.id'].format(**wildcards)
    c['PowerSupplies'][0]['RelatedItem'][0]['@odata.id']=c['PowerSupplies'][0]['RelatedItem'][0]['@odata.id'].format(**wildcards)
    c['PowerSupplies'][1]['@odata.id']=c['PowerSupplies'][1]['@odata.id'].format(**wildcards)
    c['PowerSupplies'][1]['RelatedItem'][0]['@odata.id']=c['PowerSupplies'][1]['RelatedItem'][0]['@odata.id'].format(**wildcards)
    c['PowerSupplies'][2]['@odata.id']=c['PowerSupplies'][2]['@odata.id'].format(**wildcards)

    return c
