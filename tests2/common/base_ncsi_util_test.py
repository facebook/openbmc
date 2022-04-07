#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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
import re
from enum import Enum, unique

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


@unique
class Comparison(Enum):
    LESS_THAN = 1
    LESS_THAN_OR_EQUAL = 2
    EQUAL = 3
    GREATER_THAN_OR_EQUAL = 4
    GREATER_THAN = 5


# Requirements are just a way to encapsulate the expected values from adapter
# status in a programmatic way.
# These should be initialized in platform-specific code.
# The base test provides some hopefully sane defaults.
class Requirement:
    def __init__(self, field: str, value: int, comparison: Comparison):
        self.field = field
        self.value = value
        self.comparison = comparison

    def check(self, result_dict):
        if self.field not in result_dict:
            return (False, "Field " + self.field + "not found")
        if self.comparison == Comparison.LESS_THAN:
            if not (result_dict[self.field] < self.value):
                return (
                    False,
                    "Field "
                    + self.field
                    + " read value "
                    + str(result_dict[self.field])
                    + ", expected less than "
                    + str(self.value),
                )
        if self.comparison == Comparison.LESS_THAN_OR_EQUAL:
            if not (result_dict[self.field] <= self.value):
                return (
                    False,
                    "Field "
                    + self.field
                    + " read value "
                    + str(result_dict[self.field])
                    + ", expected less than or equal to  "
                    + str(self.value),
                )
        if self.comparison == Comparison.EQUAL:
            if not (result_dict[self.field] == self.value):
                return (
                    False,
                    "Field "
                    + self.field
                    + " read value "
                    + str(result_dict[self.field])
                    + ", expected equal to "
                    + str(self.value),
                )
        if self.comparison == Comparison.GREATER_THAN_OR_EQUAL:
            if not (result_dict[self.field] >= self.value):
                return (
                    False,
                    "Field "
                    + self.field
                    + " read value "
                    + str(result_dict[self.field]_
                    + ", expected greater than or equal to "
                    + str(self.value),
                )
        if self.comparison == Comparison.GREATER_THAN:
            if not (result_dict[self.field] > self.value):
                return (
                    False,
                    "Field "
                    + self.field
                    + " read value "
                    + str(result_dict[self.field])
                    + ", expected greater than "
                    + str(self.value),
                )

        return (True, "Field " + self.field + " passed")


class BaseNcsiUtilTest(object):
    adapter_fields = None

    def setUp(self):
        print("in BaseNcsiUtilTest setUp")
        Logger.start(name=self._testMethodName)
        self.link_status = None
        self.adapter_status = None

        # Expected response and status/speed for link status
        self.link_response = None
        self.link_up_status = None
        self.link_speed = None

        # Expected response and fields for adapter status fields
        self.adapter_response = None
        if self.adapter_fields is None:
            self.adapter_fields = []  # THIS MUST BE A LIST OF Requirement OBJECTS
        self.adapter_field_results = {}


class CommonNcsiUtilTest(BaseNcsiUtilTest):
    def verify_command_reponse(self, command_response, expected_response):
        response_regex = re.compile(r"Response: (\S*) Reason: (\S*)")
        for line in command_response:
            response_m = response_regex.match(line)
            if response_m:
                self.assertTrue(
                    (response_m.group(1) == expected_response),
                    "NC-SI command gave unexpected response "
                    + response_m.group(1)
                    + " - reason is "
                    + response_m.group(2),
                )

    def get_link_status(self):
        if self.link_status is None:
            self.link_status = run_shell_cmd("ncsi-util 0xa")

    # Standard-ish output from link status
    # root@sled2002-oob:~# ncsi-util 0xa
    # NC-SI Command Response:
    # cmd: GET_LINK_STATUS(0xa)
    # Response: COMMAND_COMPLETED(0x0000)  Reason: NO_ERROR(0x0000)

    # Link Flag: Up
    # Speed and duplex: 100Gbps

    def verify_link_status(self):
        self.get_link_status()
        self.verify_command_reponse(self.link_status, self.link_response)
        link_regex = re.compile(r"Link Flag: (\S*)")
        speed_regex = re.compile(r"Speed and duplex: (\S*)")
        for line in self.link_response:
            link_m = link_regex.match(line)
            speed_m = speed_regex.match(line)
            if link_m:
                self.assertTrue((link_m.group(1) == self.link_up_status))
            if speed_m:
                self.assertTrue((speed_m.group(1) == self.link_speed))

    def get_adapter_status(self):
        if self.adapter_status is None:
            self.adapter_status = run_shell_cmd("ncsi-util -S")

    # example adapter status

    # root@sled2002-oob:~# ncsi-util -S
    # NC-SI Command Response:
    # cmd: GET_CONTROLLER_PACKET_STATISTICS(0x18)
    # Response: COMMAND_COMPLETED(0x0000)  Reason: NO_ERROR(0x0000)

    # NIC statistics
    #   Counters cleared last read (MSB): 0
    #   Counters cleared last read (LSB): 0
    #   Total Bytes Received: 3841298404
    #   Total Bytes Transmitted: 1193895696
    #   Total Unicast Packet Received: 9849545
    #   Total Multicast Packet Received: 120891
    #   Total Broadcast Packet Received: 300
    #   Total Unicast Packet Transmitted: 9296813
    #   Total Multicast Packet Transmitted: 2596
    #   Total Broadcast Packet Transmitted: 1678
    #   FCS Receive Errors: 0
    #   Alignment Errors: 0
    #   False Carrier Detections: 0
    #   Runt Packets Received: 0
    #   Jabber Packets Received: 0
    #   Pause XON Frames Received: 0
    #   Pause XOFF Frames Received: 0
    #   Pause XON Frames Transmitted: 0
    #   Pause XOFF Frames Transmitted: 0
    #   Single Collision Transmit Frames: 0
    #   Multiple Collision Transmit Frames: 0
    #   Late Collision Frames: 0
    #   Excessive Collision Frames: 0
    #   Control Frames Received: 0
    #   64-Byte Frames Received: 0
    #   65-127 Byte Frames Received: 6122225
    #   128-255 Byte Frames Received: 193099
    #   256-511 Byte Frames Received: 1883023
    #   512-1023 Byte Frames Received: 63142
    #   1024-1522 Byte Frames Received: 1709247
    #   1523-9022 Byte Frames Received: 0
    #   64-Byte Frames Transmitted: 0
    #   65-127 Byte Frames Transmitted: 8791982
    #   128-255 Byte Frames Transmitted: 242212
    #   256-511 Byte Frames Transmitted: 78570
    #   512-1023 Byte Frames Transmitted: 20261
    #   1024-1522 Byte Frames Transmitted: 168062
    #   1523-9022 Byte Frames Transmitted: 0
    #   Valid Bytes Received: 9970736
    #   Error Runt Packets Received: 0
    #   Error Jabber Packets Received: 0

    # creates a dict from the fields after NIC statistics (format is Field Name: value)
    def create_adapter_fields(self, adapter_response: str):
        header_string = "NIC statistics"
        header_position = self.adapter_status.index(header_string)
        if header_position < 0:
            return
        fields_string = self.adapter_status[header_position + len(header_string) :]

        # This captures the field name (first non-space char to the :)
        # and value (number after the :)
        field_regex = re.compile(r"\s*([^:]*): ([0-9]+)")
        for line in fields_string.split("\n"):
            field_result = field_regex.match(line)
            if field_result is not None:  # just skip invalid lines
                self.adapter_field_results[field_result.group(1)] = int(
                    field_result.group(2)
                )

    def verify_adapter_status(self):
        self.get_adapter_status()
        self.verify_command_reponse(self.adapter_status, self.adapter_response)
        self.create_adapter_fields(self.adapter_response)
        for field in self.adapter_fields:
            ret = field.check(self.adapter_field_results)
            self.assertTrue(ret[0], ret[1])

    # get broadcom core dump:
    # /usr/local/bin/ncsi-util -n eth0 -m brcm -d coredump -o <output file>

    # get broadcom crash dump
    # /usr/local/bin/ncsi-util -m brcm -d crashdump -o <output file>

    # get mellanox core dump:
    # ncsi-util -m nvidia -d devdump -o <output file>

    # This checks the link status and adapter status - platforms will need
    # to define the expected responses as defined in the test object.
    # You NEED to define link_response, link_up_status, link_speed, and
    # adapter_response. You probably should have some Requirements for
    # adapter_fields, but they're not required.
    # Some defaults are set here, but platform owners are encouraged to override them.

    def test_ncsi_util(self):
        if self.link_response is None:
            self.link_response = "COMMAND_COMPLETED(0x0000)"
        if self.link_up_status is None:
            self.link_up_status = "Up"
        if self.link_speed is None:
            self.link_speed = "100Gbps"
        if self.adapter_response is None:
            self.adapter_response = "COMMAND_COMPLETED(0x0000)"
        self.verify_link_status()
        self.verify_adapter_status()
