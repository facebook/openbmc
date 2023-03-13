# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Thermal Template File

import copy

_TEMPLATE = \
    {
        "@odata.context": "{rb}$metadata#Chassis/Links/Members/{ch_id}/Links/Thermal/$entity",
        "@odata.id": "{rb}Chassis/{ch_id}/Thermal",
        "@odata.type": "#Thermal.v1_0_0.Thermal",
        "Id": "Thermal",
        "Name": "Thermal Metrics",
        "Temperatures": [
            {
                "@odata.id": "{rb}Chassis/{ch_id}/Thermal#/Temperatures/0",
                "MemberId": "0",
                "Name": "CPU1 Temp",
                "SensorNumber": 42,
                "Status": {
                    "State": "Enabled",
                    "Health": "OK"
                },
                "ReadingCelcius": 21,
                "UpperThresholdNonCritical": 42,
                "UpperThresholdCritical": 42,
                "UpperThresholdFatal": 42,
                "LowerThresholdNonCritical": 42,
                "LowerThresholdCritical": 5,
                "LowerThresholdFatal": 42,
                "MinimumValue": 0,
                "MaximumValue": 200,
                "PhysicalContext": "CPU",
                "RelatedItem": [
                    {"@odata.id": "{rb}Systems/{ch_id}#/Processors/0" }
                ]
            },
            {
                "@odata.id": "{rb}Chassis/{ch_id}/Thermal#/Temperatures/1",
                "MemberId": "1",
                "Name": "CPU2 Temp",
                "SensorNumber": 43,
                "Status": {
                    "State": "Enabled",
                    "Health": "OK"
                },
                "ReadingCelsius": 21,
                "UpperThresholdNonCritical": 42,
                "UpperThresholdCritical": 42,
                "UpperThresholdFatal": 42,
                "LowerThresholdNonCritical": 42,
                "LowerThresholdCritical": 5,
                "LowerThresholdFatal": 42,
                "MinReadingRange": 0,
                "MaxReadingRange": 200,
                "PhysicalContext": "CPU",
                "RelatedItem": [
                    {"@odata.id": "{rb}Systems/{ch_id}#/Processors/1" }
                ]
            },
            {
                "@odata.id": "{rb}Chassis/{ch_id}/Thermal#/Temperatures/2",
                "MemberId": "2",
                "Name": "Chassis Intake Temp",
                "SensorNumber": 44,
                "Status": {
                    "State": "Enabled",
                    "Health": "OK"
                },
                "ReadingCelsius": 25,
                "UpperThresholdNonCritical": 30,
                "UpperThresholdCritical": 40,
                "UpperThresholdFatal": 50,
                "LowerThresholdNonCritical": 10,
                "LowerThresholdCritical": 5,
                "LowerThresholdFatal": 0,
                "MinReadingRange": 0,
                "MaxReadingRange": 200,
                "PhysicalContext": "Intake",
                "RelatedItem": [
                    {"@odata.id": "{rb}Chassis/{ch_id}" },
                    {"@odata.id": "{rb}Systems/{ch_id}" }
                ]
            }
        ],
        "Fans": [
            {
                "@odata.id":"{rb}Chassis/{ch_id}/Thermal#/Fans/0",
                "MemberId":"0",
                "FanName": "BaseBoard System Fan",
                "PhysicalContext": "Backplane",
                "Status": {
                    "State": "Enabled",
                    "Health": "OK"
                },
                "ReadingRPM": 2100,
                "UpperThresholdNonCritical": 42,
                "UpperThresholdCritical": 4200,
                "UpperThresholdFatal": 42,
                "LowerThresholdNonCritical": 42,
                "LowerThresholdCritical": 5,
                "LowerThresholdFatal": 42,
                "MinReadingRange": 0,
                "MaxReadingRange": 5000,
                "Redundancy" : [
                    {"@odata.id": "{rb}Chassis/{ch_id}/Thermal#/Redundancy/0"}
                ],
                "RelatedItem" : [
                    {"@odata.id": "{rb}Systems/{ch_id}" },
                    {"@odata.id": "{rb}Chassis/{ch_id}" }
                ]
            },
            {
                "@odata.id": "{rb}Chassis/{ch_id}/Thermal#/Fans/1",
                "MemberId": "1",
                "FanName": "BaseBoard System Fan Backup",
                "PhysicalContext": "Backplane",
                "Status": {
                    "State": "Enabled",
                    "Health": "OK"
                },
                 "ReadingRPM": 2100,
                "UpperThresholdNonCritical": 42,
                "UpperThresholdCritical": 4200,
                "UpperThresholdFatal": 42,
                "LowerThresholdNonCritical": 42,
                "LowerThresholdCritical": 5,
                "LowerThresholdFatal": 42,
                "MinReadingRange": 0,
                "MaxReadingRange": 5000,
                "Redundancy" : [
                    {"@odata.id": "{rb}Chassis/{ch_id}/Power#/Redundancy/0"}
                ],
                "RelatedItem" : [
                    {"@odata.id": "{rb}Systems/{ch_id}" },
                    {"@odata.id": "{rb}Chassis/{ch_id}" }
                ]
            }
        ],
        "Redundancy": [
            {
                "@odata.id": "{rb}Chassis/{ch_id}/Thermal#/Redundancy/0",
                "MemberId": "0",
                "Name": "BaseBoard System Fans",
                "RedundancySet": [
                    { "@odata.id": "{rb}Chassis/{ch_id}/Thermal#/Fans/0" },
                    { "@odata.id": "{rb}Chassis/{ch_id}/Thermal#/Fans/1" }
                ],
                "Mode": "N+1",
                "Status": {
                    "State": "Enabled",
                    "Health": "OK"
                },
                "MinNumNeeded": 1,
                "MaxNumSupported": 2
            }
        ]
    }


def get_thermal_instance(wildcards):
    """
    Returns a formatted template

    Arguments:
        rest_base - Base URL of the RESTful interface
        ident     - Identifier of the chassis
    """
    c = copy.deepcopy(_TEMPLATE)

    c['@odata.context'] = c['@odata.context'].format(**wildcards)
    c['@odata.id'] = c['@odata.id'].format(**wildcards)
    c['Redundancy'][0]['@odata.id']=c['Redundancy'][0]['@odata.id'].format(**wildcards)
    c['Fans'][0]['@odata.id']=c['Fans'][0]['@odata.id'].format(**wildcards)
    c['Fans'][1]['@odata.id']=c['Fans'][1]['@odata.id'].format(**wildcards)
    c['Temperatures'][0]['@odata.id']=c['Temperatures'][0]['@odata.id'].format(**wildcards)
    c['Temperatures'][0]['RelatedItem'][0]['@odata.id']=c['Temperatures'][0]['RelatedItem'][0]['@odata.id'].format(**wildcards)
    c['Temperatures'][1]['@odata.id']=c['Temperatures'][1]['@odata.id'].format(**wildcards)
    c['Temperatures'][1]['RelatedItem'][0]['@odata.id']=c['Temperatures'][1]['RelatedItem'][0]['@odata.id'].format(**wildcards)
    c['Temperatures'][2]['@odata.id']=c['Temperatures'][2]['@odata.id'].format(**wildcards)
    c['Temperatures'][2]['RelatedItem'][0]['@odata.id']=c['Temperatures'][2]['RelatedItem'][0]['@odata.id'].format(**wildcards)
    c['Temperatures'][2]['RelatedItem'][1]['@odata.id']=c['Temperatures'][2]['RelatedItem'][1]['@odata.id'].format(**wildcards)
    c['Fans'][0]['Redundancy'][0]['@odata.id']=c['Fans'][0]['Redundancy'][0]['@odata.id'].format(**wildcards)
    c['Fans'][0]['RelatedItem'][0]['@odata.id']=c['Fans'][0]['RelatedItem'][0]['@odata.id'].format(**wildcards)
    c['Fans'][0]['RelatedItem'][1]['@odata.id']=c['Fans'][0]['RelatedItem'][1]['@odata.id'].format(**wildcards)
    c['Fans'][1]['Redundancy'][0]['@odata.id']=c['Fans'][1]['Redundancy'][0]['@odata.id'].format(**wildcards)
    c['Fans'][1]['RelatedItem'][0]['@odata.id']=c['Fans'][1]['RelatedItem'][0]['@odata.id'].format(**wildcards)
    c['Fans'][1]['RelatedItem'][1]['@odata.id']=c['Fans'][1]['RelatedItem'][1]['@odata.id'].format(**wildcards)
    c['Redundancy'][0]['RedundancySet'][0]['@odata.id']=c['Redundancy'][0]['RedundancySet'][0]['@odata.id'].format(**wildcards)
    c['Redundancy'][0]['RedundancySet'][1]['@odata.id']=c['Redundancy'][0]['RedundancySet'][1]['@odata.id'].format(**wildcards)

    return c
