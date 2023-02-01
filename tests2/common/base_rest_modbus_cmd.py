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
import errno
import json
import os
import socketserver
import struct
import subprocess
import threading
import time
import urllib.request
from unittest import TestCase

RACKMOND_SOCKET = "/var/run/rackmond.sock"
MODBUS_CMD_URL = "http://localhost:8080/api/sys/modbus/cmd"

# Common use case for microbench (read 6 PSU registers)
EXAMPLE_PAYLOAD = {
    "commands": [
        [164, 3, 0, 122, 0, 16],
        [165, 3, 0, 122, 0, 16],
        [166, 3, 0, 122, 0, 16],
        [180, 3, 0, 122, 0, 16],
        [181, 3, 0, 122, 0, 16],
        [182, 3, 0, 122, 0, 16],
    ],
    "expected_response_length": 37,
    "custom_timeout": 500,
}

# SLA for this endpoint for one request with 6 commands has a p99 of 3s
MODBUS_CMD_P99_SLA = 3

MICROBENCH_SAMPLE_SIZE = 40

MOCK_RACKMOND_RESPONSE_DATA = [1, 2, 3, 4]
MOCK_RACKMOND_RESPONSE_PAYLOAD = json.dumps(
    {"status": "SUCCESS", "data": MOCK_RACKMOND_RESPONSE_DATA}
).encode()
MOCK_RACKMOND_RESPONSE = (
    struct.pack("L", len(MOCK_RACKMOND_RESPONSE_PAYLOAD))
    + MOCK_RACKMOND_RESPONSE_PAYLOAD
)


class RestModbusCmdTest(TestCase):
    @classmethod
    def setUpClass(cls):
        # Stop the real rackmond as it's not functional in most test environments
        # Give it enough time - 30s, instead of default 10s,
        # to stop in case of a slow system
        subprocess.run(
            "sv -w 30 force-stop rackmond || systemctl stop rackmond", shell=True
        )
        try:
            os.remove(RACKMOND_SOCKET)
        except FileNotFoundError:
            pass

        # Spawn fake rackmond to listen to RACKMOND_SOCKET
        cls.mock_rackmond = MockRackmondServer()
        cls.mock_rackmond_th = threading.Thread(target=cls.mock_rackmond.serve_forever)
        cls.mock_rackmond_th.start()

        # Wait for rackmond to stop and mock rackmond to start
        time.sleep(10)

    @classmethod
    def tearDownClass(cls):
        # Stop fake rackmond
        cls.mock_rackmond.shutdown()
        cls.mock_rackmond_th.join()

        # Recover real rackmond
        subprocess.run(
            "sv start rackmond || systemctl start rackmond", shell=True, check=True
        )

        # Wait for rackmond fully up and running and mock rackmond to be
        # stopped, so the subsequent test case can stop rackmond and start
        # the mock with success
        time.sleep(20)

    def test_modbus_cmd_post(self):
        raw_payload = json.dumps(EXAMPLE_PAYLOAD).encode("utf-8")
        req = urllib.request.Request(MODBUS_CMD_URL, data=raw_payload)
        resp = urllib.request.urlopen(req)
        resp_json = json.loads(resp.read().decode("utf-8"))

        # MockRackmondRequestHandler response
        expected_response = [
            len(MOCK_RACKMOND_RESPONSE_DATA),
            0,
        ] + MOCK_RACKMOND_RESPONSE_DATA

        self.assertEqual(resp.getcode(), 200)
        self.assertEqual(
            resp_json,
            {
                "status": "OK",
                "responses": [
                    expected_response for _ in range(len(EXAMPLE_PAYLOAD["commands"]))
                ],
            },
        )

    def test_modbus_cmd_post_microbench(self):
        # Microbenchmark to test if endpoint is within our SLAs
        # Note that this is using the a fake rackmond server and the test
        # itself is running on the BMC (and itself using BMC resources),
        # but should at least give some warning if there's a significant
        # regression in response times due to REST api code
        raw_payload = json.dumps(EXAMPLE_PAYLOAD)

        # Use curl as it's much less resource intensive than urllib.request
        # (at cost of visibility on failures)
        curl_cmd = (
            ["curl", "-X", "POST", "-f", "-v", "-k"]
            + ["-w", "%{time_total}\n"]  # Write-out total each request's time
            + ["-d", raw_payload]
        )

        # Tell curl to query the endpoint MICROBENCH_SAMPLE_SIZE times
        for _ in range(MICROBENCH_SAMPLE_SIZE):
            curl_cmd += ["-o", "/dev/null", MODBUS_CMD_URL]

        try:
            curl_run = subprocess.run(
                curl_cmd,
                check=True,
                timeout=MICROBENCH_SAMPLE_SIZE * MODBUS_CMD_P99_SLA * 2,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
            )
            measurements = [float(x) for x in curl_run.stdout.split()]

        except subprocess.CalledProcessError as e:
            # CalledProcessError doesn't print stderr by default, include stderr
            # in custom exception
            raise Exception(repr(e) + ": " + e.stderr)

        p99 = sorted(measurements)[int(MICROBENCH_SAMPLE_SIZE * 0.99)]

        # Ensure the maximum measured latency is less than our external P99 SLA
        self.assertLessEqual(
            p99,
            MODBUS_CMD_P99_SLA,
            "P99 latency of {MODBUS_CMD_URL} ({p99}s) exceeds P99 SLA of {MODBUS_CMD_P99_SLA}s".format(  # noqa: B950
                MODBUS_CMD_URL=repr(MODBUS_CMD_URL),
                MODBUS_CMD_P99_SLA=MODBUS_CMD_P99_SLA,
                p99=p99,
            ),
        )


class MockRackmondServer(socketserver.ForkingMixIn, socketserver.UnixStreamServer):
    def __init__(self):
        super().__init__(RACKMOND_SOCKET, MockRackmondRequestHandler)


class MockRackmondRequestHandler(socketserver.StreamRequestHandler):
    def handle(self):
        try:
            self.request.send(MOCK_RACKMOND_RESPONSE)
        except IOError as e:
            # EPIPE will be occurred, when run cit only rest_modbus case
            # and use --repeat option to stress test with Rackmond Version 1
            # The python /etc/rackmon_config.py still being run, and send
            # the request to /var/run/rackmond.sock
            if e.errno == errno.EPIPE:
                pass
        self.request.close()
