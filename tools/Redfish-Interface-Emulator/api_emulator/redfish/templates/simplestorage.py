# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md


from copy import deepcopy

_TEMPLATE = {u'@odata.context': '{rb}$metadata#SimpleStorage.SimpleStorage',
             u'@odata.id': "{rb}{suffix}/{suffix_id}/SimpleStorage/{storage_id}",
             u'@odata.type': u'#SimpleStorage.v1_2_0.SimpleStorage',
             u'Description': 'Simple Storage',
             u'Devices': [],
             u'Id': '{storage_id}',
             u'Links': {u'Chassis': {'@odata.id': '{rb}Chassis/{chassis_id}'}},
             u'Name': 'Simple Storage Controller',
             u'Status': {'Health': 'OK', 'State': 'Enabled'},
             u'UefiDevicePath': 'ACPI(PnP)/PCI(1,0)/SAS(0x31000004CF13F6BD,0, SATA)'}

_DEVICE_TEMPLATE = {u'CapacityBytes': 550292684800,
                    u'Manufacturer': 'Generic',
                    u'Model': 'Generic SATA Disk',
                    u'Name': 'Generic Storage Device',
                    u'Status': {'Health': 'OK', 'State': 'Enabled'}}


def format_storage_template(**kwargs):
    """
    Format the processor template -- returns the template
    """
    # params:
    defaults = {'rb': '/redfish/v1/',
                'suffix': 'Systems',
                'capacitygb': 512,
                'drives': 1}

    defaults.update(kwargs)

    c = deepcopy(_TEMPLATE)
    c['@odata.context'] = c['@odata.context'].format(**defaults)
    c['@odata.id'] = c['@odata.id'].format(**defaults)
    c['Id'] = c['Id'].format(**defaults)
    c['Links']['Chassis']['@odata.id'] = c['Links']['Chassis']['@odata.id'].format(**defaults)
    drives = []
    for i in range(defaults['drives']):
        drive = deepcopy(_DEVICE_TEMPLATE)
        drive['CapacityBytes'] = defaults['capacitygb'] * 1024 ** 3
        drive['Name'] = 'Disk %d' % i
        drives.append(drive)
    c['Devices'] = drives
    return c
