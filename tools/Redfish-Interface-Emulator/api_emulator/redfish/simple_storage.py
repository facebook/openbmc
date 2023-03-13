# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# SimpleStorage module based on SimpleStorage.0.9.2
from api_emulator import utils
from api_emulator.redfish.resource import StateEnum


class Device(object):
    """
    Device based on SimpleStorage.Device in SimpleStorage.0.9.2 (Redfish)
    """
    def __init__(self, name, status, manufacturer=None, model=None, size=None, oem=None):
        """
        Device Constructor
        """
        self._config = {
            'Name': name,
            'Status': status}

        if manufacturer is not None:
            self._config['Manufacturer'] = manufacturer
        if model is not None:
            self._config['Model'] = model
        if oem is not None:
            self._config['OEM'] = oem
        if size is not None:
            self.size, self.units = size.split(' ')
            self.size = int(self.size)
            self._config['Size'] = size
        else:
            self.size = 0
            self.units = 'GB'

        self.storage_extent_id = None

    def __getitem__(self, key):
        return self.configuration[key]

    @property
    def configuration(self):
        """
        Configuration property
        """
        config = self._config.copy()
        if self.storage_extent_id is not None:
            config['Links'] = {
                'StorageExtents': {'@odata.id': self.storage_extent_id}}
        return config


class SimpleStorage(object):
    """
    SimpleStorage based on SimpleStorage.SimpleStorage in SimpleStorage0.9.2
    (Redfish).
    """
    def __init__(self):
        """
        SimpleStorage Constructor
        """
        self.devices = []
        self.odata_id = None
        self.initialized = False

    def __getitem__(self, key):
        return self.configuration[key]

    @utils.check_initialized
    def init_odata_id(self, odata_id, system_dir, system_url):
        """
        Initialize via an OData ID
        """
        self.odata_id = odata_id

        self._config = utils.process_id(odata_id, system_dir, system_url)

        devices = self._config['Devices']

        for device in devices:
            kwargs = {}

            if 'OEM' in device:
                kwargs['oem'] = device['OEM']
            if 'Manufacturer' in device:
                kwargs['manufacturer'] = device['Manufacturer']
            if 'Size' in device:
                kwargs['size'] = device['Size']
            if 'Model' in device:
                kwargs['model'] = device['Model']

            self.devices.append(Device(device['Name'], device['Status'], **kwargs))

        del self._config['Devices']
        self.initialized = True

    @utils.check_initialized
    def init_config(self, item_id, parent_cs_puid, status, devices, rest_base, system_suffix):
        """
        Initialize via configuration

        Arguments:
            item_id        - Index of the SimpleStorage object
            parent_cs_puid - CS_PUID of the ComputerSystem it belongs to
            status         - Status of the SimpleStorage object
            devices        - Devices associated with the SimpleStorage object
                             return by the device() method
            system_suffix  - Either PooledNodes or Systems, this is used in
                             making the odata content
        """
        self.devices = devices
        self.odata_id = '{0}{1}/{2}/SimpleStorage/{3}'.format(rest_base, system_suffix, parent_cs_puid, item_id)
        self._config = {
            '@odata.context': '{0}$metadata#{1}/{2}/SimpleStorage/$entity'.format(rest_base, system_suffix, parent_cs_puid),
            '@odata.id': self.odata_id,
            'Id': str(item_id),
            'Name': 'Simple Storage Controller',
            'Description': 'System SATA',
            'UEFIDevicePath': 'UEFI Device Path',
            'Status': status}
        self.initialized = True

    @property
    def configuration(self):
        """
        Configuration property
        """
        config = self._config.copy()
        devices = []

        for device in self.devices:
            devices.append(device.configuration)

        config['Devices'] = devices

        return config


class SimpleStorageCollection(object):
    """
    SimpleStorage0.9.2.SimpleStorageCollection (Redfish) Class
    """
    def __init__(self, rest_base, system_suffix):
        """
        SimpleStorage Constructor

        Arguments:
            rest_base     - Base REST URL
            system_suffix - Either PooledNodes or Systems, this is used in
                              making the odata content
        """
        self._config = None
        self.odata_id = None
        self.storage_gb = 0
        self.disk_count = 0
        self.members_ids = []
        self.members = []  # The zeroth element is never assigned
        self.rest_base = rest_base
        self.suffix = system_suffix
        self.cs_puid = None

        # Internal variable to not allow one of the init_* methods to be
        # called multiple times.
        self.initialized = False

    @property
    def configuration(self):
        config = self._config.copy()
        config['Links'] = {
            "Members@odata.count": len(self.members_ids),
            "Members": self.members_ids
        }
        return config

    def __getitem__(self, idx):
        """
        Get item returns the member at index idx.
        """
        return self.members[idx - 1]

    @utils.check_initialized
    def init_odata_id(self, odata_id, system_dir, system_url):
        """
        Populate the SimpleStorage object from the OData ID.

        Arguments:
            odata_id   - OData ID in the system configuration
            system_dir - Directory of the index.html where the system resides
            rest_base  - Chunk of the URL to remove
        """
        self._config = utils.process_id(odata_id, system_dir, system_url)
        self.storage_gb = 0
        self.odata_id = odata_id

        self.member_ids = self._config['Links']['Members']

        # Populating the members list
        for member in self.member_ids:
            ss = SimpleStorage()
            ss.init_odata_id(member['@odata.id'], system_dir, system_url)
            self.members.append(ss)

        # Calculating amount of storage
        self._calculate_storage()

        # Updating the "Modified" key
        self._config['Modified'] = utils.timestamp()
        self.initialized = True

    @utils.check_initialized
    def init_config(self, cs_puid, storage_objects=None):
        """
        Populate the SimpleStorage object via a configuration.

        Arguments:
            cs_puid         - CS_PUID of the system the storage object
                              belongs to
            storage_objects - SimpleStorage objects to add to the collection
        """
        self.cs_puid = cs_puid
        self.odata_id = '{0}{1}/{2}/SimpleStorage'.format(self.rest_base, self.suffix, cs_puid)

        self._config = {
            '@odata.context': '{0}$metadata#{1}/{2}/SimpleStorage(*,Links/Members/$ref)'.format(self.rest_base, self.suffix, cs_puid),
            '@odata.type': '#SimpleStorage.1.00.0.SimpleStorageCollection',
            'Name': 'Simple Storage Collection',
            'MemberType': '#SimpleStorage.1.00.0.SimpleStorage'
        }

        if storage_objects is not None:
            self.append(storage_objects)
        self.initialized = True

    def append(self, storage_objects):
        """
        Add the given Devices to the SimpleStorage object. The devices
        parameter must be a list of objects made with the Device() method,
        i.e. they must be according to the SimpleStorage0.9.2 spec.
        """
        for obj in storage_objects:
            self.members.append(obj)
            idx = self.members.index(obj) + 1
            self.members_ids.append(
                {'@odata.id': '{0}{1}/{2}/SimpleStorage/{3}'.format(
                    self.rest_base, self.suffix, self.cs_puid, idx)})
        self._calculate_storage()

    def _calculate_storage(self):
        """
        Private method to calculate the amount of storage given the devices
        for the SimpleStorage object.
        """
        # Resetting disk and storage amounts
        self.disk_count = 0
        self.storage_gb = 0

        for member in self.members:
            if member is None:
                continue  # Skip the first member of the list

            devices = member['Devices']
            self.disk_count += len(devices)

            for device in devices:
                if device['Status']['State'] != StateEnum.ABSENT:
                    size = device['Size']
                    if 'GB' in size:
                        size = int(size.strip('GB'))
                    elif 'TB' in size:
                        size = 1000 * int(size.strip('TB'))
                    else:
                        raise RuntimeError('Unknown drive size amount: ' + size)
                    self.storage_gb += size
