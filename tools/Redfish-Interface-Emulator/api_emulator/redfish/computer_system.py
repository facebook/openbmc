# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# ComputerSystem module based on ComputerSystem.0.9.2

from uuid import uuid4

from . import simple_storage
from . import simple_network
from . import resource
from .templates.redfish_computer_system import REDFISH_TEMPLATE
from api_emulator.exceptions import CreatePooledNodeError
from api_emulator import utils

from .processor import Processors, Processor

from threading import Thread
from time import sleep

import collections

class ResetWorker(Thread):
    def __init__(self, states, cs):
        super(ResetWorker, self).__init__()
        self.states = states
        self.cs = cs

    def run(self):
        self.cs.config['PowerState'] = self.states[0]
        sleep(10)
        self.cs.config['PowerState'] = self.states[1]


class ComputerSystem(object):
    """
    Pooled node based on the ComputerSystem.1.00.0.ComputerSystem
    """
    def __init__(self, config, cs_puid, rest_base, suffix):
        """
        ComputerSystem Constructor

        Note: If OVF is set to True, then config must be of type <insert OVF type>
        <ADD PARAM DOCUMENTATION>
        """
        self.suffix = suffix
        self.cs_puid = cs_puid
        self.rb = rest_base
        self.config = {}
        self.processor_count = 0
        self.total_memory_gb = 0

        self.SimpleNetwork = simple_network.EthernetNetworkInterfaceCollection(self.rb)
        self.EthernetInterfaces= simple_network.EthernetNetworkInterfaceCollection(self.rb)
        self.SimpleStorage = simple_storage.SimpleStorageCollection(self.rb, suffix)
        self.Processors = Processors(rest_base, suffix, cs_puid)

        self.SimpleNetwork.init_config(self.cs_puid, suffix)
        self.EthernetInterfaces.init_config(self.cs_puid,suffix)
        self.SimpleStorage.init_config(self.cs_puid)

        self.configure(config)

    @property
    def configuration(self):
        config = self.config.copy()

        if self.SimpleNetwork.members:
            config['Links']['SimpleNetwork'] = \
                {'@odata.id': self.SimpleNetwork.odata_id}
        if self.EthernetInterfaces.members:
            config['Links']['EthernetInterfaces'] = \
                {'@odata.id': self.EthernetInterfaces.odata_id}
        if self.SimpleStorage.members:
            config['Links']['SimpleStorage'] = \
                {'@odata.id': self.SimpleStorage.odata_id}

        return self.config

    @property
    def storage_gb(self):
        """
        Return the amount of storage the system has in GB
        """
        return self.SimpleStorage.storage_gb

    @property
    def network_ports(self):
        """
        Return the number of VLAN network ports in the system
        """
        return self.SimpleNetwork.port_count

    def configure(self, config):
        """
        Overridden configure() method
        """
        self._base_configure()
        self.config['Processors']['Count'] = int(config['NumberOfProcessors'])
        self.config['Links']['Processors'] = {'@odata.id': self.Processors.odata_id}
        self.processor_count = int(config['NumberOfProcessors'])
        self._create_processors(self.processor_count)

        self.config['Memory']['TotalSystemMemoryGB'] = int(config['TotalSystemMemoryGB'])
        self.total_memory_gb = self.config['Memory']['TotalSystemMemoryGB']

        if 'Boot' in config:
            self.config['Boot'] = config['Boot']

        self.add_network_ports(int(config['NumberOfNetworkPorts']))
        self.add_storage(config['Devices'])

    def reboot(self, config):
        state = config['PowerState']

        if state == 'On':
            states = [ 'On', 'Off' ]
        elif state == 'Off':
            states = [ 'Off', 'On' ]
        else:
            raise CreatePooledNodeError('Incorrect PowerState value.')

        ResetWorker(states, self).start()

    def _replace_config(self, config, change):
        for key, value in change.iteritems():
            if isinstance(value, collections.Mapping):
                ret = self._replace_config(config.get(key, {}), value)
                config[key] = ret
            else:
                config[key] = change[key]
        return config

    def update_config(self,change):
        self.config = self._replace_config(self.config, change)

    def add_network_ports(self, amount):
        """
        Add network ports to the pooled node

        Arguments:
            amount - Number of ports to add
        """
        status = resource.Status(resource.StateEnum.ENABLED, resource.HealthEnum.OK)
        network_objs = []
        for i in range(amount):
            config = simple_network.EthernetNetworkInterface.create_config(
                self.rb, self.suffix, self.cs_puid, i + 1, status, None, 100)
            eth = simple_network.EthernetNetworkInterface(config, None)
            network_objs.append(eth)
        self.SimpleNetwork.add_network_objects(network_objs)
        self.EthernetInterfaces.add_network_objects(network_objs)

    def add_storage(self, device_list):
        """
        Add the given devices to the storage for the pooled node.

        NOTE: The device_list should be a list of dictionaries that can be
        turned into SimpleStorage objects
        """
        devices = []

        # Creating a SimpleStorage object
        for dev in device_list:
            status = resource.Status(
                state=resource.StateEnum.ENABLED,
                health=resource.HealthEnum.OK)
            device = simple_storage.Device(
                name=dev['Name'],
                manufacturer='N/A',
                model='N/A',
                size=dev['Size'],
                status=status,
                oem={})
            devices.append(device)

        # Creating SimpleStorage objects
        ss = simple_storage.SimpleStorage()
        ss.init_config(
            item_id=1,
            parent_cs_puid=self.cs_puid,
            status=resource.Status(
                resource.StateEnum.ENABLED,
                resource.HealthEnum.OK,
                resource.HealthEnum.OK),
            devices=devices,
            rest_base=self.rb,
            system_suffix=self.suffix)
        storage_objects = [ss]
        self.SimpleStorage.append(storage_objects)

    def _create_processors(self, count):
        """
        Populates the Processors attribute
        """
        status = resource.Status(resource.StateEnum.ENABLED,
                                 resource.HealthEnum.OK)
        for i in range(count):
            p = Processor(self.rb, self.suffix, self.cs_puid,
                          i + 1, i + 1, 3700, 8, 4, 4, 2, status)
            self.Processors.add_processor(p)

    def _base_configure(self):
        """
        Private method to do a base configuration of the pooled node
        """
        try:
            self.config = REDFISH_TEMPLATE.copy()
            self.odata_id = self.config['@odata.id'].format(
                rest_base=self.rb, cs_puid=self.cs_puid)
            self.config['@odata.context'] = self.config['@odata.context'].format(
                rest_base=self.rb, cs_puid=self.cs_puid)
            self.config['@odata.id'] = self.odata_id
            self.config['Actions']['#ComputerSystem.Reset']['target'] = \
                self.config['Actions']['#ComputerSystem.Reset']['target'].format(
                    cs_puid=self.cs_puid)
            self.config['Id'] = self.cs_puid

        except KeyError as e:
            raise CreatePooledNodeError(
                'Incorrect configuration, missing key: ' + e.message)
