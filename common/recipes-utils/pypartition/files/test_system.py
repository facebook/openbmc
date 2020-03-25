# Copyright 2017-present Facebook. All Rights Reserved.
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

# Intended to compatible with both Python 2.7 and Python 3.x.
from __future__ import absolute_import, division, print_function, unicode_literals

import errno
import logging
import os
import subprocess
import textwrap
import unittest

import system


try:
    from unittest.mock import MagicMock, call, mock_open, patch
except ImportError:
    from mock import MagicMock, call, mock_open, patch


class TestSystem(unittest.TestCase):
    def setUp(self):
        self.logger = MagicMock()
        self.mock_image = MagicMock()
        self.mock_image.size = 1
        self.mock_image.file_name = "/tmp/myimagefile"
        self.mock_mtd = MagicMock()
        self.mock_mtd.size = 1
        self.mock_mtd.file_name = "/dev/mtd99"
        self.mock_mtd.offset = 0

    @patch.object(system, "open", create=True)
    def test_is_openbmc(self, mocked_open):
        for (data, is_magic) in [
            (b"this is a unit test\n", False),
            (b"\\S\n", False),
            (b"OpenBMC Release wedge-v51\n", True),
            (b"Open BMC Release v25 \\n \\l\n", True),
        ]:
            mocked_open.return_value = mock_open(read_data=data).return_value
            self.assertEqual(system.is_openbmc(), is_magic)

    @patch.object(system, "open", create=True)
    def test_get_mtds_single_full_mode(self, mocked_open):
        for name in ["flash0", "flash", "Partition_000"]:
            data = textwrap.dedent(
                """\
            dev:    size   erasesize  name
            mtd0: 02000000 00010000 "{}"
            """
            ).format(name)
            mocked_open.return_value = mock_open(read_data=data).return_value
            for mtds in system.get_mtds():
                self.assertEqual(len(mtds), 1)
                self.assertEqual(mtds[0].file_name, "/dev/mtd0")
                self.assertEqual(mtds[0].size, 0x2000000)
                self.assertEqual(mtds[0].device_name, name)

    @patch.object(system, "get_vboot_enforcement", return_value="none")
    @patch.object(system, "open", create=True)
    def test_get_mtds_dual_super_fit_mode(self, mocked_open, mocked_get_vboot):
        for order in [(0, 1), (1, 0)]:
            data = textwrap.dedent(
                """\
                dev:    size   erasesize  name
                mtd0: 00060000 00010000 "romx"
                mtd1: 00020000 00010000 "env"
                mtd2: 00060000 00010000 "u-boot"
                mtd3: 01b20000 00010000 "fit"
                mtd4: 00400000 00010000 "data0"
                mtd5: 02000000 00010000 "flash{}"
                mtd6: 00060000 00010000 "rom"
                mtd7: 00020000 00010000 "envro"
                mtd8: 00060000 00010000 "u-bootro"
                mtd9: 01b20000 00010000 "fitro"
                mtd10: 00400000 00010000 "dataro"
                mtd11: 02000000 00010000 "flash{}"
            """
            ).format(order[0], order[1])
            mocked_open.return_value = mock_open(read_data=data).return_value
            (full_mtds, all_mtds) = system.get_mtds()
            self.assertEqual(len(full_mtds), 2)
            self.assertEqual(full_mtds[0].device_name, "flash1")
            self.assertEqual(len(all_mtds), 12)

    @patch.object(subprocess, "check_call")
    @patch.object(system, "open", create=True)
    def test_get_writeable_mounted_mtds(self, mocked_open, mocked_check_call):
        head = textwrap.dedent(
            """\
            rootfs / rootfs rw 0 0
            proc /proc proc rw 0 0
            sysfs /sys sysfs rw 0 0
            """
        )
        device = "/dev/mtdblock99"
        mountpoint = "/mnt/data"
        mtd_mount = "{} {} jffs2 {} 0 0\n".format(device, mountpoint, "{}")
        tail = textwrap.dedent(
            """\
            tmpfs /run tmpfs rw,nosuid,nodev,mode=755 0 0
            tmpfs /var/volatile tmpfs rw 0 0
            devpts /dev/pts devpts rw,gid=5,mode=620 0 0"""
        )
        self.mock_mtd.device_name = "data0"
        for options in [None, "rw", "ro"]:
            middle = mtd_mount.format(options) or ""
            data = head + middle + tail
            mocked_open.return_value = mock_open(read_data=data).return_value
            mtds = system.get_writeable_mounted_mtds()
            if options == "rw":
                self.assertEqual(len(mtds), 1)
                self.assertEqual(mtds[0], (device, mountpoint))
            else:
                self.assertEqual(len(mtds), 0)

    @patch.object(subprocess, "check_call")
    def test_fuser_k_mount_ro(self, mocked_check_call):
        writeable_mounted_mtds = []
        calls = []
        # TODO set ProcessError side effect
        for i in range(3):
            mocked_check_call.reset_mock()
            system.fuser_k_mount_ro(writeable_mounted_mtds, self.logger)
            mocked_check_call.assert_has_calls(calls)
            writeable_mounted_mtds.append(
                ("/dev/mtdblock{}".format(i), "/mnt/foo{}".format(i))
            )
            calls.append(call(["fuser", "-km", "/mnt/foo{}".format(i)]))
            calls.append(
                call(
                    [
                        "mount",
                        "-o",
                        "remount,ro",
                        "/dev/mtdblock{}".format(i),
                        "/mnt/foo{}".format(i),
                    ]
                )
            )

    @patch.object(subprocess, "check_call", autospec=True)
    def test_remove_healthd_reboot(self, mocked_check_call):
        data = """{"version":"1.0","heartbeat":{"interval":500},"bmc_cpu_utilization":{"enabled":true,"window_size":120,"monitor_interval":1,"threshold":[{"value":80,"hysteresis":5,"action":["log-critical","bmc-error-trigger"]}]},"bmc_mem_utilization":{"enabled":true,"enable_panic_on_oom":false,"window_size":120,"monitor_interval":1,"threshold":[{"value":60,"hysteresis":5,"action":["log-warning"]},{"value":70,"hysteresis":5,"action":["log-critical","bmc-error-trigger"]},{"value":80,"hysteresis":5,"action":["log-critical","reboot"]}]},"i2c":{"enabled":false,"busses":[0,1,2,3,4,5,6,7,8,9,10,11,12,13]},"ecc_monitoring":{"enabled":false,"ecc_address_log":false,"monitor_interval":2,"recov_max_counter":255,"unrec_max_counter":15,"recov_threshold":[{"value":0,"action":["log-critical","bmc-error-trigger"]},{"value":50,"action":["log-critical"]},{"value":90,"action":["log-critical"]}],"unrec_threshold":[{"value":0,"action":["log-critical","bmc-error-trigger"]},{"value":50,"action":["log-critical"]},{"value":90,"action":["log-critical"]}]},"bmc_health":{"enabled":false,"monitor_interval":2,"regenerating_interval":1200},"verified_boot":{"enabled":false}}"""  # noqa: E501
        # cov couldn't get a decorator version to surface .write()
        with patch("system.open", new=mock_open(read_data=data)) as mock_file, patch(
            "time.sleep"
        ):
            system.remove_healthd_reboot(self.logger)
            self.assertNotIn(call().write('"reboot"'), mock_file.mock_calls)
        mocked_check_call.assert_called_with(["sv", "restart", "healthd"])

    # If this had autospec=True, assert_not_called() wouldn't work
    # https://bugs.python.org/issue28380.
    @patch.object(subprocess, "check_call")
    @patch.object(system, "open", create=True)
    def test_remove_healthd_reboot_no_config(self, mocked_open, mocked_check_call):
        mocked_open.side_effect = IOError(
            errno.ENOENT, os.strerror(errno.ENOENT), "/etc/healthd-config.json"
        )
        system.remove_healthd_reboot(self.logger)
        self.assertIn(call("/etc/healthd-config.json"), mocked_open.mock_calls)
        mocked_check_call.assert_not_called()

    @patch.object(subprocess, "check_output")
    def test_flash_too_big(self, mocked_check_output):
        self.mock_image.size = 2
        with self.assertRaises(SystemExit) as context_manager:
            system.flash(1, self.mock_image, self.mock_mtd, self.logger)
            mocked_check_output.assert_not_called()
            self.assertEqual(context_manager.exception.code, 1)

    @patch.object(system, "open", create=True)
    @patch.object(
        system, "glob", return_value=["/proc/123/cmdline", "/proc/self/cmdline"]
    )
    @patch.object(os, "getpid", return_value=123)
    def test_other_flasher_running_skip_self(
        self, mocked_getpid, mocked_glob, mocked_open
    ):
        self.assertEqual(system.other_flasher_running(self.logger), False)
        mocked_open.assert_not_called()

    @patch.object(system, "open", create=True)
    @patch.object(system, "glob")
    @patch.object(os, "getpid", return_value=123)
    def test_other_flasher_running_return_false(
        self, mocked_getpid, mocked_glob, mocked_open
    ):
        read_data = [
            b"runsv\x00/etc/sv/restapi\x00",
            b"/usr/bin/mTerm_server\x00wedge\x00/dev/ttyS1\x00",
            b"/usr/sbin/ntpd\x00-p\x00/var/run/ntpd.pid\x00-g\x00",
            b"\x00",
        ]
        # Return the full list of /proc/$pid/cmdline from a single glob() call
        mocked_glob.return_value = [
            "/proc/{}/cmdline".format(200 + i) for i in range(len(read_data))
        ]
        # Return just one cmdline from each open().read()
        mocked_open.side_effect = [
            mock_open(read_data=datum).return_value for datum in read_data
        ]
        self.assertEqual(system.other_flasher_running(self.logger), False)

    @patch.object(system, "open", create=True)
    @patch.object(system, "glob", return_value=["/proc/456/cmdline"])
    @patch.object(os, "getpid", return_value=123)
    def test_other_flasher_running_return_true(
        self, mocked_getpid, mocked_glob, mocked_open
    ):
        read_data = [
            b"python\x00/usr/local/bin/psu-update-delta.py\x00",
            b"jbi\x00-i\x00/tmp/WEDGE100_SYSCPLD_v6_101.jbc\x00",
        ]
        for datum in read_data:
            mocked_open.return_value = mock_open(read_data=datum).return_value
            self.assertEqual(system.other_flasher_running(self.logger), True)

    @patch.object(system, "other_flasher_running", return_value=True)
    @patch.object(subprocess, "check_call")
    def test_flash_while_flashing_refused(
        self, mocked_check_call, mocked_other_flasher_running
    ):
        with self.assertRaises(SystemExit) as context_manager:
            system.flash(1, self.mock_image, self.mock_mtd, self.logger)
            mocked_check_call.assert_not_called()
            self.assertEqual(context_manager.exception.code, 1)

    @patch.object(system, "other_flasher_running", return_value=False)
    def test_flash_retries(self, mocked_other_flasher_running):
        for combination in range(100):
            attempts = combination % 10
            failures = combination // 10
            if attempts > failures + 1:
                continue
            # Create a new one every iteration, instead of reusing a single
            # patch decorator instance, to make debugging failed call count
            # assertions simple.
            with patch.object(subprocess, "check_call") as mocked_check_call:
                return_values = [MagicMock()]
                flashcp = call(
                    ["flashcp", self.mock_image.file_name, self.mock_mtd.file_name]
                )
                calls = []
                for _ in range(min(attempts, failures)):
                    return_values.insert(
                        0, subprocess.CalledProcessError(255, "in", "out")
                    )
                    calls.append(flashcp)
                mocked_check_call.side_effect = return_values
                system.flash(attempts, self.mock_image, self.mock_mtd, self.logger)
                self.assertNotEqual(mocked_other_flasher_running.call_count, 0)
                if attempts == 0:
                    self.assertEqual(mocked_check_call.call_count, 1)
                    self.assertNotEqual(mocked_check_call.call_args[0][0], "flashcp")
                else:
                    mocked_check_call.assert_has_calls(calls)

    @patch.object(system, "other_flasher_running", return_value=True)
    @patch.object(subprocess, "call")
    def test_reboot_while_flashing_refused(
        self, mocked_call, mocked_other_flasher_running
    ):
        with self.assertRaises(SystemExit) as context_manager:
            system.reboot(False, "testing", self.logger)
            mocked_call.assert_not_called()
            self.assertEqual(context_manager.exception.code, 1)

    @patch.object(system, "other_flasher_running", return_value=False)
    @patch.object(subprocess, "call")
    @patch.object(logging, "shutdown")
    def test_reboot(self, mocked_shutdown, mocked_call, mocked_other_flasher_running):
        inputs_outputs = [
            (
                True,
                ["wall", "pypartition would be testing if this were not a dry run."],
            ),
            (False, ["shutdown", "-r", "now", "pypartition is testing."]),
        ]
        for (dry_run, expected_arguments) in inputs_outputs:
            with self.assertRaises(SystemExit):
                system.reboot(dry_run, "testing", self.logger)
                mocked_call.assert_called_once_with(expected_arguments)
                mocked_shutdown.assert_called()
