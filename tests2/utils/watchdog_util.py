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

from time import sleep

from utils.shell_util import run_shell_cmd


class WatchdogUtils:
    """Watchdog utility functions.
    """

    _DEVMEM_CMD = "/sbin/devmem"
    _WDT1_CTRL_REG = "0x1E78500C"
    _WDT1_STATUS_REG = "0x1E785000"
    _WDTCLI_CMD = "/usr/local/bin/wdtcli"

    def start_watchdog(self):
        """Start the watchdog.
        """
        cmd = self._WDTCLI_CMD + " start"
        run_shell_cmd(cmd)

    def stop_watchdog(self):
        """Stop the watchdog.
        """
        cmd = self._WDTCLI_CMD + " stop"
        run_shell_cmd(cmd)

    def kick_watchdog(self):
        """Kick the watchdog.
        """
        cmd = self._WDTCLI_CMD + " kick"
        run_shell_cmd(cmd)

    def _read_ctrl_register(self):
        """Read watchdog control register.
        """
        cmd = self._DEVMEM_CMD + " " + self._WDT1_CTRL_REG
        cmd_out = run_shell_cmd(cmd)
        return int(cmd_out, 16)

    def _read_status_register(self):
        """Read watchdog status register.
        """
        cmd = self._DEVMEM_CMD + " " + self._WDT1_STATUS_REG
        cmd_out = run_shell_cmd(cmd)
        return int(cmd_out, 16)

    def _watchdog_is_ticking(self):
        """Check if watchdog timer is ticking.
        """
        # NOTE: please make sure the watchdog is not touched by other
        # apps when this function is called; otherwise, the function
        # may return incorrect result because the watchdog might be
        # restarted by other apps.
        count_1 = self._read_status_register()

        # Sleep for a short while so the watchdog controller gets chance
        # to refresh the status register.
        sleep(0.01)  # 10 milliseconds

        count_2 = self._read_status_register()

        if count_2 < count_1:
            return True
        else:
            return False

    def watchdog_is_running(self, check_counter=False):
        """Check if watchdog is running.
        """
        # Check if the last 2 bits of control register are set.
        #  - bit 0: set to enable watchdog.
        #  - bit 1: set to reboot system when timer expires.
        reg_val = self._read_ctrl_register()
        if reg_val & 3 != 3:
            return False

        if check_counter and not self._watchdog_is_ticking():
            return False

        return True
