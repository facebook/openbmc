# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md


from copy import deepcopy
from random import randint

_TEMPLATE = {u'@odata.context': '{rb}$metadata#EthernetInterface.EthernetInterface',
             u'@odata.id': "{rb}{suffix}/{suffix_id}/EthernetInterfaces/{nic_id}",
             u'@odata.type': u'#EthernetInterface.v1_3_0.EthernetInterface',
             u'AutoNeg': True,
             u'Description': 'Ethernet Interface',
             u'FQDN': 'default.local',
             u'FullDuplex': True,
             u'HostName': 'default',
             u'IPv4Addresses': [{u'Address': "'172.16.%d.%d'%(randint(1,255),randint(1,255))",
                                 u'AddressOrigin': 'IPv4LinkLocal',
                                 u'Gateway': '0.0.0.0',
                                 u'SubnetMask': '255.255.0.0'}],
             u'Id': '{nic_id}',
             u'InterfaceEnabled': True,
             u'LinkStatus': 'LinkUp',
             u'Links': {u'Chassis': {'@odata.id': '{rb}Chassis/{chassis_id}'}},
             u'MACAddress': "",
             u'MTUSize': 1500,
             u'Name': 'Ethernet Interface',
             u'NameServers': ['8.8.8.8'],
             u'PermanentMACAddress': "':'.join(['%02x'%randint(0,255) for x in range(6)])",
             u'SpeedMbps': 10000,
             u'Status': {'Health': 'OK', 'State': 'Enabled'},
             u'VLAN': {u'VLANId': 0}}


def format_nic_template(**kwargs):
    """
    Format the processor template -- returns the template
    """
    # params:
    defaults = {'rb': '/redfish/v1/',
                'suffix': 'Systems',
                'speedmbps': 10000,
                'vlanid': 0}

    defaults.update(kwargs)

    c = deepcopy(_TEMPLATE)
    c['@odata.context'] = c['@odata.context'].format(**defaults)
    c['@odata.id'] = c['@odata.id'].format(**defaults)
    c['Id'] = c['Id'].format(**defaults)
    c['Links']['Chassis']['@odata.id'] = c['Links']['Chassis']['@odata.id'].format(**defaults)
    c['SpeedMbps'] = defaults['speedmbps']
    for ip in c['IPv4Addresses']:
        ip['Address']=eval(ip['Address'])
    c['PermanentMACAddress']=eval(c['PermanentMACAddress'])
    c['MACAddress']=c['PermanentMACAddress']
    c['VLAN']['VLANId']=defaults['vlanid']
    return c
