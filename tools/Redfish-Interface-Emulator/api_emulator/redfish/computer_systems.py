# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# ComputerSystems Module

rest_base='/redfish/v1'

class ComputerSystemCollection(object):
    """
    PooledNodeCollection Class
    """

    def __init__(self, rest_base):
        """
        PooledNodeCollection Constructor
        """
        #self.modified = timestamp()
        self.systems = {}

    def __getitem__(self, idx):
        print ('Index is...%s...'%idx)
        return self.systems[idx]

    @property
    def configuration(self):
        """
        Configuration Property
        """
        systems = []

        for pn in self.systems:
            print (pn)
            pn = self.systems[pn]
            systems.append({'@odata.id': pn.odata_id})

        return {
            '@odata.context': '/redfish/v1/$metadata#Systems',
            '@odata.type': '#ComputerSystemCollection.ComputerSystemCollection',
            '@odata.id': '/redfish/v1/Systems',
            'Name': 'Computer System Collection',

            'Links': {
                'Members@odata.count': len(systems),
                'Members': systems
            }
        }

    @property
    def count(self):
        """
        Number of pooled nodes
        """
        return len(self.systems.keys())

    def add_computer_system(self, cs):
        """
        Add the given ComputerSystem to the collection
        """
        self.systems[cs.cs_puid] = cs

    def remove_computer_system(self, cs):
        """
        Removing the given ComputerSystem
        """
        del self.systems[cs.cs_puid]
