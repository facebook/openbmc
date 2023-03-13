# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# format_processor_template()
from copy import deepcopy

PROCESSOR_TEMPLATE = {
    '@odata.context': '{rb}$metadata#Processor.Processor',
    "@odata.type": "#Processor.1_0_0.Processor",
    "@odata.id": "{rb}{suffix}/{suffix_id}/Processors/{processor_id}",
    "Name": "Processor",
    "Id": "{processor_id}",
    "ProcessorType": "CPU",
    "ProcessorArchitecture": "x86",
    "InstructionSet": "x86-64",
    "Manufacturer": "Intel(R) Corporation",
    "Model": "Multi-Core Intel(R) Xeon(R) processor 7500 Series",
    "ProcessorId": {
        "VendorId": "GenuineIntel",
        "IdentificationRegisters": "0x34AC34DC8901274A",
        "EffectiveFamily": "0x42",
        "EffectiveModel": "0x61",
        "Step": "0x1",
        "MicrocodeInfo": "0x429943"
    },
    'Links': {'Chassis': {'@odata.id': '{rb}Chassis/{chassis_id}'}},
    "MaxSpeedMHz": None,
    "TotalCores": None,
    "TotalThreads": None,
    "Status": {'Health': 'OK', 'State': 'Enabled'}
}


def format_processor_template(**kwargs):
    """
    Format the processor template -- returns the template
    """
    # params:
    defaults = {'rb': '/redfish/v1/',
                'suffix': 'Systems',
                'maxspeedmhz': 2200,
                'totalcores': 8}

    defaults.update(kwargs)

    c = deepcopy(PROCESSOR_TEMPLATE)
    c['@odata.context'] = c['@odata.context'].format(**defaults)
    c['@odata.id'] = c['@odata.id'].format(**defaults)
    c['Id'] = c['Id'].format(**defaults)
    c['Links']['Chassis']['@odata.id'] = c['Links']['Chassis']['@odata.id'].format(**defaults)
    c['MaxSpeedMHz'] = defaults['maxspeedmhz']
    c['TotalCores'] = defaults['totalcores']
    c['TotalThreads'] = defaults.get('totalthreads',2*defaults['totalcores'])
    return c
