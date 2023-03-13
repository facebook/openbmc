# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# SimpleNetwork module based on EthernetNetworkInterface.0.94.0 (Redfish)

from api_emulator import utils


def vlan(rest_base, suffix, cs_puid, member_id, identifier, status):
    # TODO - Add Documentation
    return {
        '@odata.context': '{0}$metadata#{1}/Links/Members/{2}/SimpleNetwork/Links/Members/{3}/Links/VLANs/Links/Members/$entity'.format(rest_base, suffix, cs_puid, member_id),
        '@odata.id': '{0}{1}/{2}/SimpleNetwork/{3}/VLANs/{4}'.format(rest_base, suffix, cs_puid, member_id, identifier),
        '@odata.type': '#VLanNetworkInterface.1.00.0.VLANNetworkInterface',
        'Id': str(identifier),
        'Name': 'VLAN Network Interface',
        'Description': 'System NIC {0} V'.format(identifier),
        'Status': status,
        'VLANEnable': True,
        'VLANId': 101,
        'IPv6First': True,
        'IPv4Address': [
            {
                'Address': '16.84.245.117',
                'SubnetMask': '255.255.252.0',
                'AddressOrigin': 'Static',
                'Gateway': '16.84.244.1',
                'HostName': 'myserver',
                'FQDN': 'myserver.mycorp.com'
            }
        ],
        'IPv6Address': [
            {
                'Address': 'fe80::1ec1:deff:fe6f:1e24',
                'PrefixLength': 64,
                'AddressOrigin': 'Static',
                'Scope': 'Site',
                'Gateway': 'fe80::3ed9:2bff:fe34:600',
                'HostName': 'myserver',
                'FQDN': 'myserver.mycorp.com'
            }
        ]
    }


class VLANs(object):
    def __init__(self, config, vlans):
        self._config = config
        self.vlans = vlans

    def __getitem__(self, idx):
        return self.vlans[idx - 1]

    @property
    def configuration(self):
        return self._config

    @staticmethod
    def create_config(rest_base, suffix, cs_puid, member_id, vlans):
        """
        Creates a configuration to be given to the constructor.

        Arguments:
            <ADD DOCUMENTATION>
        """
        member_ids = []
        for member in vlans:
            member_ids.append({'@odata.id': member['@odata.id']})

        return {
            '@odata.context': '{0}$metadata#{1}/Links/Members/{2}/Links/SimpleNetwork/Links/Members/{3}/Links/VLANs/$entity'.format(rest_base, suffix, cs_puid, member_id),
            '@odata.id': '{0}{1}/{2}/SimpleNetwork/{3}/VLANs'.format(rest_base, suffix, cs_puid, member_id),
            '@odata.type': '#VLanNetworkInterface.1.00.0.VLANNetworkInterfaceCollection',
            'Name': 'VLAN Network Interface Collection',
            'Links': {
                'Members@odata.count': len(member_ids),
                'Members': member_ids
            }
        }


class EthernetNetworkInterface(object):
    """
    EthernetNetworkInterface based on EthernetNetworkInterface.0.94.0
    """
    def __init__(self, config, VLANs):
        self._config = config

        # if VLANs is not None:
        self.VLANs = VLANs

    @staticmethod
    def create_config(rest_base, suffix, cs_puid, identifier, status, vlans, speed):
        """
        Static method to create an EthernetNetworkInterface configuration.

        Returns a dictionary that should be given to the constructor.

        <ADD ARGUMENT DOCUMENTATION>
        """
        if vlans is not None:
            vlans = {
                '@odata.id': '{0}{1}/{2}/SimpleNetwork/{3}/VLANs'.format(rest_base, suffix, cs_puid, identifier)}
        else:
            vlans = {}

        return {
            '@odata.context': '{0}$metadata#{1}/Links/Members/{2}/Links/SimpleNetwork/Links/Members/$entity'.format(rest_base, suffix, cs_puid),
            '@odata.id': '{0}{1}/{2}/SimpleNetwork/{3}'.format(rest_base, suffix, cs_puid, identifier),
            '@odata.type': '#EthernetNetworkInterface.1.00.0.EhternetNetworkInterface',
            'Id': str(identifier),
            'Name': 'Ethernet Network Interface',
            'Description': 'System NIC {0}'.format(identifier),
            'Status': status,
            'FactoryMacAddress': 'AA:BB:CC:DD:EE:FF',
            'MacAddress': 'AA:BB:CC:DD:EE:FF',
            'LinkTechnology': 'Ethernet',
            'Speed': speed,
            'FullDuplex': True,
            'HostName': 'web483',
            'FQDN': 'web483.redfishspecification.org',
            'IPv6DefaultGateway': 'fe80::3ed9:2bff:fe34:600',
            'NameServers': [
                'names.redfishspecification.org'
            ],
            'IPv4Address': [
                {
                    'Address': '192.168.0.10',
                    'SubnetMask': '255.255.252.0',
                    "AddressOrigin": 'Static',
                    'Gateway': '192.168.0.1'
                }
            ],
            'IPv6Address': [
                {
                    'Address': 'fe80::1ec1:deff:fe6f:1e24',
                    'PrefixLength': 64,
                    'AddressOrigin': 'Static',
                    'AddressState': 'Preferred'
                }
            ],
            'Links': {
                'VLANs': vlans,
                'Oem': {}
            }
        }

    @property
    def configuration(self):
        return self._config


class EthernetNetworkInterfaceCollection(object):
    """
    EthernetNetworkInterfaceCollection based on
    EthernetNetworkInterface.EthernetNetworkInterfaceCollection
    """
    def __init__(self, rest_base):
        """
        ADD DOCUMENTATION
        """
        # self.identifier = identifier
        self.rest_base = rest_base
        self.link_key = 'Links'
        self.odata_id = None

        self.members = []
        self.members_ids = []

        self._config = {}
        self.initialized = False

        self.vport_count = 0
        self.port_count = 0

    @property
    def configuration(self):
        config = self._config.copy()
        config[self.link_key] = {
            'Members@odata.count': len(self.members_ids),
            'Members': self.members_ids,
            'OEM': {}
        }
        return config

    def __getitem__(self, idx):
        return self.members[idx - 1]

    @utils.check_initialized
    def init_odata_id(self, odata_id, system_dir, system_url):
        """
        ADD DOCUMENTATION
        """
        self._config = utils.process_id(odata_id, system_dir, system_url)
        self.odata_id = self._config['@odata.id']

        if 'links' in self._config:
            self.link_key = 'links'

        self.members_ids = self._config[self.link_key]['Members']

        for member in self.members_ids:
            config = utils.process_id(
                member['@odata.id'], system_dir, system_url)
            vlans = None

            if 'VLANs' in config['Links']:
                vlans_config = utils.process_id(
                    config['Links']['VLANs']['@odata.id'],
                    system_dir, system_url)
                vlans_members = []

                for vlan in vlans_config['Links']['Members']:
                    vlans_members.append(
                        utils.process_id(
                            vlan['@odata.id'], system_dir, system_url))
                vlans = VLANs(vlans_config, vlans_members)

            member_obj = EthernetNetworkInterface(config, vlans)
            self.members.append(member_obj)

        self._calculate_ports()

    @utils.check_initialized
    def init_config(self, cs_puid, suffix, network_objects=None):
        """
        ADD DOCUMENTATION

        Arguments:
            cs_puid        - CS_PUID of the system the network controller
                             belongs to
            network_objects - List of network objects
            suffix          - Either PooledNodes or Systems, this is used in
                              making the odata content
        """
        self.odata_id = '{0}{1}/{2}/SimpleNetwork'.format(self.rest_base, suffix, cs_puid)
        self._config = {
            '@odata.context': '{0}$metadata#{1}/Links/Members/{2}/Links/SimpleNetwork/$entity'.format(self.rest_base, suffix, cs_puid),
            '@odata.id': self.odata_id,
            '@odata.type': '#EthernetNetworkInterface.1.00.0.EthernetNetworkInterfaceCollection',
            'Name': 'NIC Collection',
            'Description': 'Simple Network Collection'}

        if network_objects is not None:
            self.add_network_objects(network_objects)

    def add_network_objects(self, network_objects):
        """
        Add network objects to the collection

        Arguments:
            network_objects - List of EthernetN
        """
        for obj in network_objects:
            self.members_ids.append({'@odata.id': obj.configuration['@odata.id']})
            self.members.append(obj)
        self._calculate_ports()

    def _calculate_ports(self):
        """
        Calculate the available ports based on the configuration
        """
        self.port_count = len(self.members)
        self.vport_count = 0

        for member in self.members:
            if member.VLANs is not None:
                self.vport_count += len(member.VLANs.vlans)
