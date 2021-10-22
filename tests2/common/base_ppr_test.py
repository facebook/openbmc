#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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

import os
import re
import time
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


G_VERBOSE = False


class BasePPRTest(object):

    USERVER_HOSTNAME = "DEFAULT"
    BMC_HOSTNAME = "DEFAULT"
    DEFAULT_ERR_INJECT_PATH = "/sys/kernel/debug/apei/einj/"
    DEFAULT_PPR_HISTORY_FILE = "/mnt/data/ppr/fru{}_ppr_history_data"
    DEFAULT_POWER_CYCLE_CMD = "/usr/local/bin/power-util server cycle"
    DEFAULT_PRINT_SEL_CMD = "/usr/local/bin/log-util server --print"
    DEFAULT_CLEAR_SEL_CMD = "/usr/local/bin/log-util all --clear"
    DEFAULT_GET_PPR_ROW_COUNT_CMD = "/usr/local/bin/ipmi-util 0x{} 0xc0 0x91 0x2"
    DEFAULT_GET_DIMM_ROW_ADDR_CMD = "/usr/local/bin/ipmi-util 0x{} 0xc0 0x91 0x3 0x{}"
    DEFAULT_ENABLE_PPR_FUNC_CMD = "/usr/local/bin/ipmi-util 0x{} 0xc0 0x90 0x1 0x{}"
    DEFAULT_GET_PPR_ACTION_CMD = "/usr/local/bin/ipmi-util 0x{} 0xc0 0x91 0x1"
    DEFAULT_GET_PPR_HISTORY_DATA_CMD = (
        "/usr/local/bin/ipmi-util 0x{} 0xc0 0x91 0x4 0x{}"
    )
    DEFAULT_MEM_ERR_PATTERN = "Memory correctable error"
    DEFAULT_PPR_TEST_PASS_PATTERN = "PPR success"
    DEFAULT_SYSTEM_RESTART_TIME = 180
    DEFAULT_DIMM_ADDR_LEN = 8
    DEFAULT_PPR_HISTORY_DATA_LEN = 17

    def setUp(self):
        self.is_log = True
        self.bmc_hostname = None
        self.hostname = None
        self.bmc_username = None
        self.bmc_passwd = None
        self.userver_username = None
        self.userver_passwd = None
        self.ppr_row_cnt = None
        self.param1_val = None
        self.param2_val = None
        self.err_type = None
        self.err_inject = None
        self.set_hostname()
        self.set_device_config()
        self.set_platform_info()
        self.set_err_inject_info()
        self.set_command()
        Logger.start(name=self._testMethodName)

    def set_hostname(self):
        self.hostname = os.environ.get("TEST_HOSTNAME", self.USERVER_HOSTNAME)
        self.bmc_hostname = os.environ.get("TEST_BMC_HOSTNAME", self.BMC_HOSTNAME)

    @abstractmethod
    def set_device_config(self):
        pass

    @abstractmethod
    def set_platform_info(self):
        pass

    @abstractmethod
    def set_err_inject_info(self):
        pass

    def set_command(self):
        self.power_cycle = self.DEFAULT_POWER_CYCLE_CMD
        self.print_sel = self.DEFAULT_PRINT_SEL_CMD
        self.clear_sel = self.DEFAULT_CLEAR_SEL_CMD
        self.ssh_cmd_to_host = (
            "sshpass -p {} ssh -q -o StrictHostKeyChecking=no {}@{} ".format(
                self.userver_passwd,
                self.userver_username,
                self.hostname,
            )
        )
        self.ssh_cmd_to_bmc = (
            "sshpass -p {} ssh -q -o StrictHostKeyChecking=no {}@{} ".format(
                self.bmc_passwd,
                self.bmc_username,
                self.bmc_hostname,
            )
        )

    def show_info(self, logging=False):
        if logging:
            print("\n ====== PPR test information ====== \n")
            print(f"BMC IP: {self.bmc_hostname}")
            print(f"Slot Number: {self.slot_num}")
            print(f"PPR function: {self.ppr_func}")
            print(f"System DIMM Quantity: {self.dimm_qty}\n")

    def inject_error(self, logging=False):
        parameters = ["param1", "param2", "error_type", "error_inject"]

        if logging:
            print("\n ====== Inject PPR error ====== \n")

        if len(self.param1_val) < self.dimm_qty:
            self.fail("param1_val is invalid!")
        # Clear BMC SELs
        run_shell_cmd(self.ssh_cmd_to_bmc + self.clear_sel)

        # Check parameters in error inject folder
        for param in parameters:
            run_shell_cmd(
                self.ssh_cmd_to_host + f"ls {self.DEFAULT_ERR_INJECT_PATH}{param}"
            )

        # Inject error
        for i in range(self.dimm_qty):
            if logging:
                print("Inject memory error for DIMM#{}\n".format(i))
            values = [
                self.param1_val[i],
                self.param2_val,
                self.err_type,
                self.err_inject,
            ]
            for val, param in zip(values, parameters):
                run_shell_cmd(
                    self.ssh_cmd_to_host
                    + "'echo {} > {}{}'".format(
                        val, self.DEFAULT_ERR_INJECT_PATH, param
                    )
                )
                time.sleep(1)

        err_sel = run_shell_cmd(
            self.ssh_cmd_to_bmc
            + self.print_sel
            + f" | grep '{self.DEFAULT_MEM_ERR_PATTERN}'"
        )
        err_cnt = err_sel.count("Memory correctable error")
        if self.dimm_qty == err_cnt:
            if logging:
                print("Inject memory error successfully\n")
        else:
            self.fail(
                f"Inject memory error failed: the number of error SEL: {err_cnt} doesn't match with DIMM Quantity: {self.dimm_qty}"
            )

        if logging:
            print("Memory correctable error SEL in BMC:\n")
            print(err_sel)

    def get_ppr_row_count(self, logging=False):
        if logging:
            print("\n ====== Get DIMM Row Count ====== \n")

        res = run_shell_cmd(
            self.ssh_cmd_to_bmc
            + self.DEFAULT_GET_PPR_ROW_COUNT_CMD.format(self.slot_num)
        )
        row_count = int(res.split(" ")[3])

        if self.dimm_qty != row_count:
            self.fail(
                f"DIMM Quantity: {self.dimm_qty} is not equal to PPR row count: {row_count}"
            )

        if logging:
            print(row_count)
        return row_count

    def enable_ppr_function(self, logging=False):
        if self.ppr_func == "Soft":
            func = "81"
        elif self.ppr_func == "Hard":
            func = "82"
        else:
            self.fail(f"Wrong PPR function: {self.ppr_func}")

        if logging:
            print(f"\n ====== Enable {self.ppr_func} PPR function ====== \n")
        # Enable PPR Function
        run_shell_cmd(
            self.ssh_cmd_to_bmc
            + self.DEFAULT_ENABLE_PPR_FUNC_CMD.format(self.slot_num, func)
        )
        time.sleep(1)
        res = run_shell_cmd(
            self.ssh_cmd_to_bmc
            + self.DEFAULT_GET_PPR_ACTION_CMD.format(
                self.slot_num,
            )
        )
        get_ppr_func = res.split(" ")[3]

        if func != get_ppr_func:
            self.fail("Failed to enable PPR function")
        if logging:
            print("Enable PPR function successfully")

    def get_ppr_data(self, cmd=None, check_result=True, logging=False):
        if logging:
            if cmd == self.DEFAULT_GET_DIMM_ROW_ADDR_CMD:
                print("\n ====== Get DIMM Row Address ====== \n")
                output_len = self.DEFAULT_DIMM_ADDR_LEN
            elif cmd == self.DEFAULT_GET_PPR_HISTORY_DATA_CMD:
                print("\n ====== Get PPR History Data ====== \n")
                output_len = self.DEFAULT_PPR_HISTORY_DATA_LEN
        results = []
        for index in range(self.ppr_row_cnt):
            res = run_shell_cmd(self.ssh_cmd_to_bmc + cmd.format(self.slot_num, index))
            results.append(res[9:])
        for r in results:
            if len(r) > 1:
                print(r)
            if check_result:
                if len(re.findall(r"[a-fA-F0-9]{2}", r)) != output_len:
                    self.fail("result is invalid!")

    def check_ppr_test_result(self, logging=False):
        # Get PPR History Data
        self.get_ppr_data(
            cmd=self.DEFAULT_GET_PPR_HISTORY_DATA_CMD,
            check_result=False,
            logging=logging,
        )
        # Clear PPR History
        run_shell_cmd(
            self.ssh_cmd_to_bmc
            + f"rm -f {self.DEFAULT_PPR_HISTORY_FILE.format(self.slot_num)}"
        )
        time.sleep(1)

        # Reset system & Check SEL Log
        if logging:
            print(" <<< Restart System >>> \n")
        run_shell_cmd(self.ssh_cmd_to_bmc + self.power_cycle)
        time.sleep(self.DEFAULT_SYSTEM_RESTART_TIME)

        sel = run_shell_cmd(self.ssh_cmd_to_bmc + self.print_sel)
        ppr_success_cnt = sel.count(self.DEFAULT_PPR_TEST_PASS_PATTERN)
        if self.ppr_row_cnt != ppr_success_cnt:
            self.fail(
                f"PPR test failed: the SEL number: {ppr_success_cnt} doesn't match with DIMM Row Count: {self.ppr_row_cnt}"
            )
        if logging:
            print(" ====== PPR SEL LOG ====== \n")
            print(sel)

        self.get_ppr_data(cmd=self.DEFAULT_GET_PPR_HISTORY_DATA_CMD, logging=logging)

    def do_external_ppr_test(self):
        self.show_info(logging=self.is_log)
        # Inject Memory error
        self.inject_error(logging=self.is_log)
        # Get PPR Row Count
        self.ppr_row_cnt = self.get_ppr_row_count(logging=self.is_log)
        # Get DIMM Row Address
        self.get_ppr_data(cmd=self.DEFAULT_GET_DIMM_ROW_ADDR_CMD, logging=self.is_log)
        # Enable PPR function
        self.enable_ppr_function(logging=self.is_log)
        # Check PPR test result
        self.check_ppr_test_result(logging=self.is_log)
