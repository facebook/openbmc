#!/usr/bin/env python
#
# Copyright 2018-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
import unittest
import os
import json
import urllib
from abc import abstractmethod
from utils.cit_logger import Logger


class BaseRestEndpointTest(unittest.TestCase):

    CURL_CMD6 = "http://localhost:8080{}"

    def setUp(self):
        Logger.start(name=__name__)

    def tearDown(self):
        pass

    @abstractmethod
    def set_endpoint(self):
        pass

    def get_from_endpoint(self, endpointname=None):
        self.assertNotEqual(endpointname, None, "Endpoint name not set")
        cmd = BaseRestEndpointTest.CURL_CMD6.format(endpointname)
        Logger.info("Executing cmd={}".format(cmd))
        try:
            # Send request to REST service to see if it's alive
            handle = urllib.request.urlopen(cmd)
            data = handle.read()
            Logger.info("REST request returned data={}".format(data.decode('utf-8')))
            return data.decode('utf-8')
        except Exception as ex:
            raise("Failed to perform REST request for cmd={}".format(cmd))

    def test_server_httperror_code_404(self):
        '''
        Test if REST server return 404 code when url is not accessible
        '''
        with self.assertRaises(urllib.error.HTTPError):
            test_endpointname = "/api/sys/testing"
            cmd = BaseRestEndpointTest.CURL_CMD6.format(test_endpointname)
            Logger.info("Executing cmd={}".format(cmd))
            urllib.request.urlopen(cmd)

    def verify_endpoint_resource(self, endpointname, resources):
        '''
        Verify if the resources exposed from REST endpoint are accessible
        '''
        for item in resources:
            with self.subTest(item=item):
                new_endpoint = endpointname + "/" + item
                cmd = BaseRestEndpointTest.CURL_CMD6.format(new_endpoint)
                handle = urllib.request.urlopen(cmd)
                self.assertIn(handle.getcode(), [200, 202])


class CommonRestEndpointTest(BaseRestEndpointTest):
    '''
    Class to capture common test cases for common REST endpoints
    test_XXX: Include the path to endpoint in test name for ease
    of readability. Run-time discovered by python unittest framework

    Each test does the following:
    - Verify if the attributes provided by platform are returned in data
    - Verify accessibility of resources exposed
    '''

    API_ENDPOINT = "/api"
    SYS_ENDPOINT = "/api/sys"
    SENSORS_ENDPOINT = "/api/sys/sensors"
    MB_ENDPOINT = "/api/sys/mb"
    SERVER_ENDPOINT = "/api/sys/server"
    FRUID_ENDPOINT = "/api/sys/mb/fruid"
    MTERM_ENDPOINT = "/api/sys/mTerm_status"
    BMC_ENDPOINT = "/api/sys/bmc"

    # /api
    @abstractmethod
    def set_endpoint_api_attributes(self):
        self.endpoint_api_attrb = None
        pass

    def test_endpoint_api(self):
        self.set_endpoint_api_attributes()
        self.assertNotEqual(self.endpoint_api_attrb, None,
            "/api endpoint attributes not set")
        info = self.get_from_endpoint(CommonRestEndpointTest.API_ENDPOINT)
        for attrib in self.endpoint_api_attrb:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, info)
        if 'Resources' in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=CommonRestEndpointTest.API_ENDPOINT,
                resources=dict_info['Resources']
            )

    # /api/sys
    @abstractmethod
    def set_endpoint_sys_attributes(self):
        self.endpoint_sys_attrb = None
        pass

    def test_endpoint_api_sys(self):
        self.set_endpoint_sys_attributes()
        self.assertNotEqual(self.endpoint_sys_attrb, None,
            "/api/sys endpoint attributes not set")
        info = self.get_from_endpoint(CommonRestEndpointTest.SYS_ENDPOINT)
        for attrib in self.endpoint_sys_attrb:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, info)
        if 'Resources' in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=CommonRestEndpointTest.SYS_ENDPOINT,
                resources=dict_info['Resources']
            )

    # /api/sys/sensors
    @abstractmethod
    def set_endpoint_sensors_attributes(self):
        self.endpoint_sensors_attrb = None
        pass

    def test_endpoint_api_sys_sensors(self):
        self.set_endpoint_sensors_attributes()
        self.assertNotEqual(self.endpoint_sensors_attrb, None,
            "/api/sys/sensors endpoint attributes not set")
        info = self.get_from_endpoint(CommonRestEndpointTest.SENSORS_ENDPOINT)
        for attrib in self.endpoint_sensors_attrb:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, info)
        if 'Resources' in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=CommonRestEndpointTest.SENSORS_ENDPOINT,
                resources=dict_info['Resources']
            )

    # "/api/sys/mb"
    @abstractmethod
    def set_endpoint_mb_attributes(self):
        self.endpoint_mb_attrb = None
        pass

    def test_endpoint_api_sys_mb(self):
        self.set_endpoint_mb_attributes()
        self.assertNotEqual(self.endpoint_mb_attrb, None,
            "/api/sys/mb endpoint attributes not set")
        info = self.get_from_endpoint(CommonRestEndpointTest.MB_ENDPOINT)
        for attrib in self.endpoint_mb_attrb:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, info)
        if 'Resources' in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=CommonRestEndpointTest.MB_ENDPOINT,
                resources=dict_info['Resources']
            )

    # "/api/sys/server"
    @abstractmethod
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = None
        pass

    def test_endpoint_api_sys_server(self):
        self.set_endpoint_server_attributes()
        self.assertNotEqual(self.endpoint_server_attrb, None,
            "/api/sys/server endpoint attributes not set")
        info = self.get_from_endpoint(CommonRestEndpointTest.SERVER_ENDPOINT)
        for attrib in self.endpoint_server_attrb:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, info)
        if 'Resources' in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=CommonRestEndpointTest.SERVER_ENDPOINT,
                resources=dict_info['Resources']
            )

    # "/api/sys/mb/fruid"
    @abstractmethod
    def set_endpoint_fruid_attributes(self):
        self.endpoint_fruid_attrb = None
        pass

    def test_endpoint_api_sys_mb_fruid(self):
        self.set_endpoint_fruid_attributes()
        self.assertNotEqual(self.endpoint_fruid_attrb, None,
            "/api/sys/mb/fruid endpoint attributes not set")
        info = self.get_from_endpoint(CommonRestEndpointTest.FRUID_ENDPOINT)
        for attrib in self.endpoint_fruid_attrb:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, info)
        if 'Resources' in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=CommonRestEndpointTest.FRUID_ENDPOINT,
                resources=dict_info['Resources']
            )

    # "/api/sys/mTerm_status"
    @abstractmethod
    def set_endpoint_mterm_attributes(self):
        self.endpoint_mterm_attrb = None
        pass

    def test_endpoint_api_sys_mterm(self):
        self.set_endpoint_mterm_attributes()
        self.assertNotEqual(self.endpoint_mterm_attrb, None,
            "/api/sys/mTerm_status endpoint attributes not set")
        info = self.get_from_endpoint(CommonRestEndpointTest.MTERM_ENDPOINT)
        for attrib in self.endpoint_mterm_attrb:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, info)
        if 'Resources' in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=CommonRestEndpointTest.MTERM_ENDPOINT,
                resources=dict_info['Resources']
            )


    # "/api/sys/bmc"
    @abstractmethod
    def set_endpoint_bmc_attributes(self):
        self.endpoint_bmc_attrb = None
        pass

    def test_endpoint_api_sys_bmc(self):
        self.set_endpoint_bmc_attributes()
        self.assertNotEqual(self.endpoint_bmc_attrb, None,
            "/api/sys/bmc endpoint attributes not set")
        info = self.get_from_endpoint(CommonRestEndpointTest.BMC_ENDPOINT)
        for attrib in self.endpoint_bmc_attrb:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, info)
        if 'Resources' in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=CommonRestEndpointTest.BMC_ENDPOINT,
                resources=dict_info['Resources']
            )


class FbossRestEndpointTest(CommonRestEndpointTest):
    '''
    Class captures endpoints common for Fboss network gear
    '''

    FC_PRESENT_ENDPOINT = "/api/sys/fc_present"
    SLOT_ID_ENDPOINT = "/api/sys/slotid"

    # "/api/sys/fc_present"
    @abstractmethod
    def set_endpoint_fc_present_attributes(self):
        self.endpoint_fc_present_attrb = None
        pass

    def test_endpoint_api_sys_fc_present(self):
        self.set_endpoint_fc_present_attributes()
        self.assertNotEqual(self.endpoint_fc_present_attrb, None,
            "/api/sys/fc_present endpoint attributes not set")
        info = self.get_from_endpoint(FbossRestEndpointTest.FC_PRESENT_ENDPOINT)
        for attrib in self.endpoint_fc_present_attrb:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, info)
        if 'Resources' in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=FbossRestEndpointTest.FC_PRESENT_ENDPOINT,
                resources=dict_info['Resources']
            )

    # "/api/sys/slotid"
    @abstractmethod
    def set_endpoint_slotid_attributes(self):
        self.endpoint_slotid_attrb = None
        pass

    def test_endpoint_api_sys_slotid(self):
        self.set_endpoint_slotid_attributes()
        self.assertNotEqual(self.endpoint_slotid_attrb, None,
            "/api/sys/slotid endpoint attributes not set")
        info = self.get_from_endpoint(FbossRestEndpointTest.SLOT_ID_ENDPOINT)
        for attrib in self.endpoint_slotid_attrb:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, info)
        if 'Resources' in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=FbossRestEndpointTest.SLOT_ID_ENDPOINT,
                resources=dict_info['Resources']
            )
