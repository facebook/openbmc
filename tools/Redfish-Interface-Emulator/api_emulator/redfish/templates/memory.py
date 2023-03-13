# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md


from copy import deepcopy

_TEMPLATE = {'@odata.context': '{rb}$metadata#Memory.Memory',
             '@odata.id': "{rb}{suffix}/{suffix_id}/Memory/{memory_id}",
             '@odata.type': '#Memory.v1_2_0.Memory',
             'CapacityMiB': -1,
             'Name': 'Memory',
             'Id': '{memory_id}',
             'Links': {'Chassis': {'@odata.id': '{rb}Chassis/{chassis_id}'}},
             'Manufacturer': 'Generic',
             'MemoryDeviceType': '',  # DDR4
             'MemoryType': '',  # DRAM,NVDIMM_N,F,P
             'OperatingMemoryModes': [],  # Volatile,PMEM, Block
             'SerialNumber': 'TJ27JXQY',
             'Status': {'Health': 'OK', 'State': 'Enabled'},
             'VendorID': 'Generic'}


def format_memory_template(**kwargs):
    """
    Format the processor template -- returns the template
    """
    # params:
    defaults = {'rb': '/redfish/v1/',
                'suffix': 'Systems',
                'capacitymb': 16384,
                'devicetype': 'DDR4',
                'type': 'DRAM',
                'operatingmodes': ['Volatile']}

    defaults.update(kwargs)

    c = deepcopy(_TEMPLATE)
    c['@odata.context'] = c['@odata.context'].format(**defaults)
    c['@odata.id'] = c['@odata.id'].format(**defaults)
    c['Id'] = c['Id'].format(**defaults)
    c['Links']['Chassis']['@odata.id'] = c['Links']['Chassis']['@odata.id'].format(**defaults)
    c['CapacityMiB'] = defaults['capacitymb']
    c['MemoryDeviceType'] = defaults['devicetype']
    c['MemoryType'] = defaults['type']
    c['OperatingMemoryModes'] = defaults['operatingmodes']
    return c
