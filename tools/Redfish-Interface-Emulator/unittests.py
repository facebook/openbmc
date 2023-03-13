#!/usr/bin/env python2
#
# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Python unittests for the Redfish Interface Emulator

import argparse
import unittest
import requests
import json
import os
import logging
import webbrowser
import sys

class TestRedfishEmulator(unittest.TestCase):
    MODE=None
    #global address
    CONFIG = 'emulator-config.json'
    with open(CONFIG, 'r') as f:
        config = json.load(f)

    MODE = config['MODE']
    address = sys.argv[2]
    base_url = 'http://{0}/redfish/v1/'.format(address)

    unittest_data = 'unittest_data'
    log_file = None
    log_fmt = logging.Formatter(
        fmt='%(asctime)s - %(name)s - %(levelname)-8s: %(message)s',
        datefmt='%H:%M:%S %m-%d-%y')

    def assert_status(self, r, expected, logger):
        try:
            assert r.status_code == expected, 'Request failed: See log "{0}"'.format(self.log_file)
        except AssertionError:
            # Catching assertion to log the response from the REST server
            # then re-raising the exception
            logger.error('Request Failed:\n' + r.text)
            raise

    def url(self, url):
        """
        Appends the base_url to the beginning of the given URL
        """
        return self.base_url + url

    def odata_id_url(self, url):
        """
        Appends http://<address>/ to the given url
        """
        return 'http://{0}{1}'.format(self.address, url)

    def get(self, logger, url, getting):
        """
        Helper function to do a get request and log it to the specified logger
        """
        r = requests.get(url)
        self.assert_status(r, 200, logger)
        logger.info('PASS: GET of {0} successful (response below)\n {1}'.format(getting, r.text))

    def get_logger(self, name, log_file):
        """
        Helper function to create a logger object
        """
        logger = logging.getLogger(name)
        fh = logging.FileHandler(log_file, mode='w')
        fh.setFormatter(self.log_fmt)
        logger.setLevel(logging.INFO)
        logger.addHandler(fh)
        return logger

    def do_gets(self, params, logger):
        """
        Helper function to call the get() member function with the given params.
        Params must be a list of parameters to give to the get()method.
        """
        for param in params:
            self.get(logger, *param)

    def test_redfish_get_serviceroot(self):
        self.log_file = 'test-get-serviceroot.log'
        logger = self.get_logger(
            'test-get-serviceroot',
            self.log_file)

        # Parameters for the get requests
        params = [
            (self.url(''), 'Service Root')
	]
        self.do_gets(params, logger)

    def test_redfish_get_system(self):
        """
        Unit test to get resource of a system instance

        NOTE: The emulator must be in the redfish mode to run this test
        """
        self.log_file = 'test-get-system.log'
        logger = self.get_logger(
            'test-get-system',
            self.log_file)

        # Parameters for the get requests
        params = [
            (self.url('Chassis'), 'ChassiCollection'),
            (self.url('Chassis/Chassis-1'), 'Chassis instance'),

            (self.url('Systems'), 'SystemCollection'),
            (self.url('Systems/System-1'), 'Systems instance'),
            (self.url('Systems/System-1/Processors'), 'ProcessorsCollection'),
            (self.url('Systems/System-1/Memory'), 'MemoryCollection'),
            (self.url('Systems/System-1/SimpleStorage'), 'SimpleStorageCollection'),
            (self.url('Systems/System-1/EthernetInterfaces'), 'EthernetInterfacesCollection'),
            (self.url('Systems/System-1/Processors/CPU0'), 'Processor member 1'),
            (self.url('Systems/System-1/Memory/DRAM0'), 'Memory member 1'),
            (self.url('Systems/System-1/SimpleStorage/SAS-CTRL0'), 'SimpleStorage member 1'),
            (self.url('Systems/System-1/EthernetInterfaces/NIC-0'), 'EthernetInterface member 1'),

            (self.url('EventService'), 'EventService'),

            (self.url('CompositionService'), 'CompositionService'),
            (self.url('CompositionService/ResourceBlocks'), 'ResourceBlockCollection'),
            (self.url('CompositionService/ResourceBlocks/Block-1'), 'ResourceBlock member'),
            (self.url('CompositionService/ResourceBlocks/Block-1/Processors/CPU0'), 'ResourceBlock Memory member'),
            (self.url('CompositionService/ResourceBlocks/Block-1/Memory/DRAM0'), 'ResourceBlock Memory member'),
            (self.url('CompositionService/ResourceBlocks/Block-1/EthernetInterfaces/NIC-0'), 'ResourceBlock EthernetInterfaces member'),
            (self.url('CompositionService/ResourceZones'), 'ResourceZoneCollection'),
            (self.url('CompositionService/ResourceZones/Zone-1'), 'ResourceZone member')]


        self.do_gets(params, logger)

    def test_redfish_create_system_status_codes(self):
        """
        Unit test to get resource of a system instance

        NOTE: The emulator must be in the redfish mode to run this test
        """
        self.log_file = 'test-create-system-status-codes.log'
        logger = self.get_logger(
            'test-create-system-status-codes',
            self.log_file)

        with open('test-composed.json') as payload:
            headers = {'Content-Type': 'application/json'}
            r = requests.post(self.url('Systems'), data=payload, headers=headers)
            self.assert_status(r, 400, logger)

        r = requests.get(self.url('Systems/Composed-1'))
        self.assert_status(r, 404, logger)

        # Testing deleting the system instance (expect to fail with 404)
        r = requests.delete(self.url('Systems/Composed-1'))
        self.assert_status(r, 404, logger)

    def test_redfish_create_system(self):
        """
        Unit test to get resource of a system instance

        NOTE: The emulator must be in the redfish mode to run this test
        """
        self.log_file = 'test-create-system-status-codes.log'
        logger = self.get_logger(
            'test-create-system-status-codes',
            self.log_file)

        with open('test-composed-with-id.json') as payload:
            headers = {'Content-Type': 'application/json'}
            r = requests.post(self.url('Systems'), data=payload, headers=headers)
            self.assert_status(r, 201, logger)

        r = requests.get(self.url('Systems/Composed-1'))
        self.assert_status(r, 200, logger)

        r = requests.delete(self.url('Systems/Composed-1'))
        self.assert_status(r, 200, logger)

if __name__ == '__main__':
    #main(sys.argv[2:])
    parser = argparse.ArgumentParser()
    parser.add_argument('spec', choices=['Redfish', 'Chinook'], type=str,
                        help='Specification used for pooled node definition'
                             ' by the emulator')

    parser.add_argument('address', metavar='address', type=str, nargs=1,
                        help='Address to access the emulator')
    args = parser.parse_args()
    print('Testing interface at:', sys.argv[2])
    suite = unittest.TestLoader().loadTestsFromTestCase(TestRedfishEmulator)
    runner = unittest.TextTestRunner(verbosity=2)

    sub_str = 'chinook'

    if args.spec == 'Chinook':
        sub_str = 'redfish'

    for t in suite:
        if sub_str in t.id():
            setattr(t, 'setUp', lambda: t.skipTest('Emulator running using the {0} spec'.format(args.spec)))
    runner.run(suite)
