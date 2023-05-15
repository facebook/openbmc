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
import re
import time
import unittest
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.test_utils import qemu_check


try:
    from urllib.error import HTTPError

    # For Python 3+
    from urllib.request import urlopen
except ImportError:
    from urllib2 import HTTPError

    # Fall back to Python 2's urllib2
    from urllib2 import urlopen

# Some endpoints may respond with a code != 200 on tests environments
ALLOWED_NON_OK_RESPONSES = {
    re.compile("^/api/(sys|spb|iom|sled/mb)/fscd_sensor_data$"): {
        405  # This endpoint is POST only
    },
    re.compile("^/api/(sys|spb|iom|sled/mb)/ntp$"): {
        # 404 response from this endpoint is normal in lab environments
        # (as ntp servers may not be available in all lab environments)
        404,
    },
}


class BaseRestEndpointException(Exception):
    pass


class BaseRestEndpointTest(object):
    CURL_CMD6 = "http://localhost:8080{}"

    WAIT_UNTIL_REACHABLE_ENDPOINT = "http://localhost:8080/api"
    # WAIT_UNTIL_REACHABLE_TIMEOUT should be greater than the amount of time it takes
    # for the REST daemon to start (so that we cater for the worst case scenario, e.g.
    # a previous test restarting the REST api)
    WAIT_UNTIL_REACHABLE_TIMEOUT = 7 * 60

    @classmethod
    def setUpClass(cls):
        cls._try_wait_until_reachable()

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
        endpoint = BaseRestEndpointTest.CURL_CMD6.format(endpointname)
        Logger.info("Requesting GET {endpoint}".format(endpoint=endpoint))
        try:
            # Send request to REST service to see if it's alive
            handle = urlopen(endpoint)
            data = handle.read()
            Logger.info("REST request returned data={}".format(data.decode("utf-8")))
            return data.decode("utf-8")
        except Exception as ex:
            raise BaseRestEndpointException(
                "Failed to request GET {endpoint}, Exception={ex}".format(
                    endpoint=endpoint,
                    ex=repr(ex),
                )
            ) from None

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
                try:
                    handle = urlopen(cmd)

                except HTTPError as err:
                    if not self._is_allowed_non_ok(
                        http_code=err.code, endpoint=new_endpoint
                    ):
                        raise BaseRestEndpointException(
                            "HTTPError on GET {new_endpoint} : {err}".format(
                                new_endpoint=new_endpoint, err=repr(err)
                            )
                        ) from None

                else:
                    self.assertIn(
                        handle.getcode(),
                        [200, 202],
                        msg="{} missing resource {}".format(endpointname, item),
                    )

    def _is_allowed_non_ok(self, http_code, endpoint):
        for re_endpoint, allowed_codes in ALLOWED_NON_OK_RESPONSES.items():
            if re_endpoint.match(endpoint) and http_code in allowed_codes:
                return True
        return False

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
                self.assertIn(
                    attrib,
                    info,
                    msg="endpoint {} missing attrib {}".format(endpointname, attrib),
                )
        if "Resources" in info:
            dict_info = json.loads(info)
            self.verify_endpoint_resource(
                endpointname=endpointname, resources=dict_info["Resources"]
            )

    @classmethod
    def _try_wait_until_reachable(cls):
        """
        Wait until the BMC is reachable in a best-effort way. Meant to be used in
        setUpClass() to reduce the amount of false positives due to the REST api
        not being up by the time the test is scheduled to run
        """
        timedout_at = time.perf_counter() + cls.WAIT_UNTIL_REACHABLE_TIMEOUT

        last_exc = None

        # Wait WAIT_UNTIL_REACHABLE_TIMEOUT seconds until the BMC responds successfully
        while time.perf_counter() < timedout_at:
            try:
                urlopen(cls.WAIT_UNTIL_REACHABLE_ENDPOINT, timeout=5)
                break

            except OSError as e:
                last_exc = e
                time.sleep(10)

        else:
            Logger.error(
                "Could not GET {WAIT_UNTIL_REACHABLE_ENDPOINT} after {WAIT_UNTIL_REACHABLE_TIMEOUT} seconds while setting up {cls}, tests will likely fail: {last_exc}".format(  # noqa: B950
                    WAIT_UNTIL_REACHABLE_ENDPOINT=cls.WAIT_UNTIL_REACHABLE_ENDPOINT,
                    WAIT_UNTIL_REACHABLE_TIMEOUT=cls.WAIT_UNTIL_REACHABLE_TIMEOUT,
                    cls=cls,
                    last_exc=repr(last_exc),
                )
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
    BMC_ENDPOINT = "/api/sys/bmc"

    # Attributes for common endpoints
    MB_ATTRIBUTES = ["fruid"]
    FRUID_ATTRIBUTES = [
        "Assembled At",
        "Local MAC",
        "Product Asset Tag",
        "Product Name",
    ]
    FRUID_ATTRIBUTES_V4 = [
        "Version",
        "Product Name",
        "Product Part Number",
        "System Assembly Part Number",
        "Meta PCBA Part Number",
        "Meta PCB Part Number",
        "ODM/JDM PCBA Part Number",
        "ODM/JDM PCBA Serial Number",
        "Product Production State",
        "Product Version",
        "Product Sub-Version",
        "Product Serial Number",
        "System Manufacturer",
        "System Manufacturing Date",
        "PCB Manufacturer",
        "Assembled at",
        "Local MAC",
        "Extended MAC Base",
        "Extended MAC Address Size",
        "EEPROM location on Fabric",
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
        "MTD Parts",
        "Secondary Boot Triggered",
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

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
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

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_sensors(self):
        self.set_endpoint_sensors_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.SENSORS_ENDPOINT, self.endpoint_sensors_attrb
        )

    # "/api/sys/mb"
    def set_endpoint_mb_attributes(self):
        self.endpoint_mb_attrb = self.MB_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
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

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_server(self):
        self.set_endpoint_server_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.SERVER_ENDPOINT, self.endpoint_server_attrb
        )

    # "/api/sys/mb/fruid"
    def set_endpoint_fruid_attributes(self):
        self.endpoint_fruid_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_mb_fruid(self):
        self.set_endpoint_fruid_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.FRUID_ENDPOINT, self.endpoint_fruid_attrb
        )

    # "/api/sys/bmc"
    def set_endpoint_bmc_attributes(self):
        self.endpoint_bmc_attrb = self.BMC_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_bmc(self):
        self.set_endpoint_bmc_attributes()
        self.verify_endpoint_attributes(
            CommonRestEndpointTest.BMC_ENDPOINT, self.endpoint_bmc_attrb
        )

    # /api/sys/bmc - MTD Parts
    def set_endpoint_mtd_attributes(self):
        self.endpoint_mtd_attrb = ["u-boot", "env", "fit", "data0", "flash0", "flash1"]

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_bmc_mtd(self):
        self.set_endpoint_mtd_attributes()
        info = self.get_from_endpoint(CommonRestEndpointTest.BMC_ENDPOINT)
        dict_info = json.loads(info)
        for attrib in self.endpoint_mtd_attrb:
            with self.subTest(attrib=attrib):
                self.assertIn(attrib, dict_info["Information"]["MTD Parts"])

    # /api/sys/bmc - Secondary Boot Triggered
    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_bmc_secondary_boot(self):
        info = self.get_from_endpoint(CommonRestEndpointTest.BMC_ENDPOINT)
        dict_info = json.loads(info)
        self.assertFalse(dict_info["Information"]["Secondary Boot Triggered"])


class FbossRestEndpointTest(CommonRestEndpointTest):
    """
    Class captures endpoints common for Fboss network gear
    """

    # Common Fboss endpoints
    FC_PRESENT_ENDPOINT = "/api/sys/fc_present"
    FIRMWARE_INFO_ENDPOINT = "/api/sys/firmware_info/all"

    # Atributes

    # "/api/sys/fc_present"
    def set_endpoint_fc_present_attributes(self):
        self.endpoint_fc_present_attrb = ["Not Applicable"]

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fc_present(self):
        self.set_endpoint_fc_present_attributes()
        self.verify_endpoint_attributes(
            FbossRestEndpointTest.FC_PRESENT_ENDPOINT, self.endpoint_fc_present_attrb
        )

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_firmware_info_all(self):
        self.set_endpoint_firmware_info_all_attributes()
        self.assertNotEqual(self.endpoint_firmware_info_all_attrb, None)
        info = self.get_from_endpoint(FbossRestEndpointTest.FIRMWARE_INFO_ENDPOINT)
        # If we get any JSON object I'm happy for the time being
        self.assertTrue(json.loads(info))
