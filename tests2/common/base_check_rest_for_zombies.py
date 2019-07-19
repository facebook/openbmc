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
import re
import time
from typing import List
from urllib.request import urlopen

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd

from .base_rest_endpoint_test import BaseRestEndpointTest


# Tests if for zombie processes at the /api rest endpoint after recursively polling all
# rest endpoints
class BaseCheckRestForZombiesTest(object):
    def setUp(self) -> None:
        Logger.start(name=self._testMethodName)
        # number of minutes between endpoint pollings
        self.num_min = int
        # numer of times the endpoints are polled
        self.num_iter = int
        self.endpoint_name = str
        self.num_min = 1
        self.num_iter = 20
        self.endpoint_name = "/api"
        pass

    def tearDown(self) -> None:
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    # Used when subclasses want to start traversal from a different endpoint
    def set_endpoint_name(self, endpoint_name: str) -> None:
        self.endpoint_name = endpoint_name

    def traverse_all_endpoints_helper(
        self, endpoint_name: str, endpointlist: List
    ) -> None:
        info = self.get_from_endpoint(endpoint_name)
        if info is not None and "Resources" in info:
            dict_info = json.loads(info)
            for resource in dict_info["Resources"]:
                resource_endpoint_name = endpoint_name + "/" + resource
                self.traverse_all_endpoints_helper(resource_endpoint_name, endpointlist)
                endpointlist.append(resource_endpoint_name)

    def traverse_all_endpoints(self) -> None:
        endpointlist = []
        self.traverse_all_endpoints_helper(self.endpoint_name, endpointlist)

    def check_for_zombies(self) -> None:
        stream = run_shell_cmd("ps")
        zombies = {}
        for line in stream.splitlines():
            process_details = line.split()
            process_command = " ".join(process_details[4:])

            # processes that we will ignore if they are zombies
            ignored_processes = {"cat", "run", "sh", "ps"}
            found_match = False
            for process in ignored_processes:
                if re.search(process, process_command) is not None:
                    found_match = True
            if (found_match is False) and (process_details[3] == "Z"):
                zombies[process_details[0]] = process_command
        return zombies

    def test_for_restpoint_zombies(self) -> None:
        self.assertNotEqual(self.endpoint_name, None, "Endpoint name not set")
        self.traverse_all_endpoints()
        zombies = {}
        pids_and_cmds = {}
        for _i in range(self.num_iter):
            time.sleep(self.num_min * 60)
            self.traverse_all_endpoints()

            # list of zombies from the most recent poll
            current_zombies = self.check_for_zombies()

            # check if any previous zombies have been removed
            for zombie in current_zombies:

                # a pre-existing zombie still exists
                if zombie in zombies:
                    zombies[zombie] += 1

                # a new zombie has spawned
                else:
                    zombies[zombie] = 1
                    pids_and_cmds[zombie] = current_zombies[zombie]

            # remove zombies that were not seen in the last poll
            for zombie in list(zombies):
                if not (zombie in current_zombies):
                    zombies.pop(zombie, None)

        # check if there are any actual zombies
        for zombie in list(zombies):
            if zombies[zombie] < 5:
                zombies.pop(zombie, None)
                pids_and_cmds.pop(zombie)
            else:
                Logger.info(
                    "Process {} with PID {} is a zombie".format(
                        pids_and_cmds[zombie], zombie
                    )
                )
        self.assertEqual(
            len(zombies),
            0,
            "There are zombies here! These are their PIDs and CMDs: {}".format(
                pids_and_cmds
            ),
        )

    def get_from_endpoint(self, endpointname: str = None) -> None:
        self.assertNotEqual(endpointname, None, "Endpoint name not set")
        cmd = BaseRestEndpointTest.CURL_CMD6.format(endpointname)
        try:
            # Send request to REST service to see if it's alive
            handle = urlopen(cmd)
            data = handle.read()
            return data.decode("utf-8")
        except Exception as ex:
            Logger.info("Test has timed out. {}".format(ex))
