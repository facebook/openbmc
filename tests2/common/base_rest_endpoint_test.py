#!/usr/bin/env python3
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
import json
import os
from abc import abstractmethod

from utils.cit_logger import Logger


try:
    # For Python 3+
    from urllib.request import urlopen
    from urllib.error import HTTPError
except ImportError:
    # Fall back to Python 2's urllib2
    from urllib2 import urlopen
    from urllib2 import HTTPError


class BaseRestEndpointTest(object):

    CURL_CMD6 = "http://localhost:8080{}"

    def setUp(self):
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
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
            handle = urlopen(cmd)
            data = handle.read()
            Logger.info("REST request returned data={}".format(data.decode("utf-8")))
            return data.decode("utf-8")
        except Exception as ex:
            raise (
                "Failed to perform REST request for cmd={}, Exception={}".format(
                    cmd, ex
                )
            )

    def test_server_httperror_code_404(self):
        """
        Test if REST server return 404 code when url is not accessible
        """
        with self.assertRaises(HTTPError):
            test_endpointname = "/api/sys/testing"
            cmd = BaseRestEndpointTest.CURL_CMD6.format(test_endpointname)
            Logger.info("Executing cmd={}".format(cmd))
            urlopen(cmd)

    def verify_endpoint_resource(self, endpointname, resources):
        """
        Verify if the resources exposed from REST endpoint are accessible
        """
        for item in resources:
            with self.subTest(item=item):
                new_endpoint = endpointname + "/" + item
                cmd = BaseRestEndpointTest.CURL_CMD6.format(new_endpoint)
                handle = urlopen(cmd)
                if item == "ntp":
                    self.assertIn(handle.getcode(), [200, 404])
                else:
                    self.assertIn(handle.getcode(), [200, 202])

    def verify_endpoint_attributes(self, endpointname, attributes):
        """
        Verify if attributes are present in endpoint response
        """
        self.assertNotEqual(
            attributes, None, "{} endpoint attributes not set".format(endpointname)
        )
        info = self.get_from_endpoint(endpointname)
        for attrib in attributes:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, info)
        if "Resources" in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=endpointname, resources=dict_info["Resources"]
            )


class CommonRestEndpointTest(BaseRestEndpointTest):
    """
    Class to capture common test cases for common REST endpoints
    test_XXX: Include the path to endpoint in test name for ease
    of readability. Run-time discovered by python unittest framework

    Each test does the following:
    - Verify if the attributes provided by platform are returned in data
    - Verify accessibility of resources exposed
    """

    API_ENDPOINT = "/api"
    SYS_ENDPOINT = "/api/sys"
    SENSORS_ENDPOINT = "/api/sys/sensors"
    MB_ENDPOINT = "/api/sys/mb"
    SERVER_ENDPOINT = "/api/sys/server"
    FRUID_ENDPOINT = "/api/sys/mb/fruid"
    MTERM_ENDPOINT = "/api/sys/mTerm_status"
    BMC_ENDPOINT = "/api/sys/bmc"

    # Attrbutes for common endpoints
    MB_ATTRIBUTES = ["fruid"]
    FRUID_ATTRIBUTES = [
        "Assembled At",
        "Local MAC",
        "Product Asset Tag",
        "Product Name",
    ]

    BMC_ATTRIBUTES = [
        "At-Scale-Debug Running",
        "CPU Usage",
        "Description",
        "MAC Addr",
        "Memory Usage",
        "OpenBMC Version",
        "SPI0 Vendor",
        "SPI1 Vendor",
        "TPM FW version",
        "TPM TCG version",
        "Reset Reason",
        "Uptime",
        "kernel version",
        "load-1",
        "load-15",
        "load-5",
        "open-fds",
        "u-boot version",
        "vboot",
    ]

    # /api
    def set_endpoint_api_attributes(self):
        self.endpoint_api_attrb = ["sys"]

    def test_endpoint_api(self):
        self.set_endpoint_api_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.API_ENDPOINT, self.endpoint_api_attrb
        )

    # /api/sys
    @abstractmethod
    def set_endpoint_sys_attributes(self):
        self.endpoint_sys_attrb = None
        pass

    def test_endpoint_api_sys(self):
        self.set_endpoint_sys_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.SYS_ENDPOINT, self.endpoint_sys_attrb
        )

    # /api/sys/sensors
    @abstractmethod
    def set_endpoint_sensors_attributes(self):
        self.endpoint_sensors_attrb = None
        pass

    def test_endpoint_api_sys_sensors(self):
        self.set_endpoint_sensors_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.SENSORS_ENDPOINT, self.endpoint_sensors_attrb
        )

    # "/api/sys/mb"
    def set_endpoint_mb_attributes(self):
        self.endpoint_mb_attrb = self.MB_ATTRIBUTES

    def test_endpoint_api_sys_mb(self):
        self.set_endpoint_mb_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.MB_ENDPOINT, self.endpoint_mb_attrb
        )

    # "/api/sys/server"
    @abstractmethod
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = None
        pass

    def test_endpoint_api_sys_server(self):
        self.set_endpoint_server_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.SERVER_ENDPOINT, self.endpoint_server_attrb
        )

    # "/api/sys/mb/fruid"
    def set_endpoint_fruid_attributes(self):
        self.endpoint_fruid_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_mb_fruid(self):
        self.set_endpoint_fruid_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.FRUID_ENDPOINT, self.endpoint_fruid_attrb
        )

    # "/api/sys/mTerm_status"
    def set_endpoint_mterm_attributes(self):
        self.endpoint_mterm_attrb = ["Running"]

    def test_endpoint_api_sys_mterm(self):
        self.set_endpoint_mterm_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.MTERM_ENDPOINT, self.endpoint_mterm_attrb
        )

    # "/api/sys/bmc"
    def set_endpoint_bmc_attributes(self):
        self.endpoint_bmc_attrb = self.BMC_ATTRIBUTES
        pass

    def test_endpoint_api_sys_bmc(self):
        self.set_endpoint_bmc_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.BMC_ENDPOINT, self.endpoint_bmc_attrb
        )


class FbossRestEndpointTest(CommonRestEndpointTest):
    """
    Class captures endpoints common for Fboss network gear
    """

    # Common Fboss endpoints
    FC_PRESENT_ENDPOINT = "/api/sys/fc_present"
    SLOT_ID_ENDPOINT = "/api/sys/slotid"
    FIRMWARE_INFO_ENDPOINT = "/api/sys/firmware_info/all"

    # Atributes

    # "/api/sys/fc_present"
    def set_endpoint_fc_present_attributes(self):
        self.endpoint_fc_present_attrb = ["Not Applicable"]

    def test_endpoint_api_sys_fc_present(self):
        self.set_endpoint_fc_present_attributes()
        self.verify_endpoint_attributes(
            FbossRestEndpointTest.FC_PRESENT_ENDPOINT, self.endpoint_fc_present_attrb
        )

    # "/api/sys/slotid"
    def set_endpoint_slotid_attributes(self):
        self.endpoint_slotid_attrb = ["0"]

    def test_endpoint_api_sys_slotid(self):
        self.set_endpoint_slotid_attributes()
        self.verify_endpoint_attributes(
            FbossRestEndpointTest.SLOT_ID_ENDPOINT, self.endpoint_slotid_attrb
        )

    def test_endpoint_api_sys_firmware_info_all(self):
        self.set_endpoint_firmware_info_all_attributes()
        self.assertNotEqual(self.endpoint_firmware_info_all_attrb, None)
        info = self.get_from_endpoint(FbossRestEndpointTest.FIRMWARE_INFO_ENDPOINT)
        # If we get any JSON object I'm happy for the time being
        self.assertTrue(json.loads(info))
