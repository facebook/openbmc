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
from __future__ import absolute_import
from __future__ import print_function
from __future__ import unicode_literals
from __future__ import division

import logging
import os
import subprocess
import system
import textwrap
import unittest

try:
    from unittest.mock import MagicMock, call, mock_open, patch
except ImportError:
    from mock import MagicMock, call, mock_open, patch


class TestSystem(unittest.TestCase):
    def setUp(self):
        self.logger = MagicMock()
        self.mock_image = MagicMock()
        self.mock_image.size = 1
        self.mock_image.file_name = '/tmp/myimagefile'
        self.mock_mtd = MagicMock()
        self.mock_mtd.size = 1
        self.mock_mtd.file_name = '/dev/mtd99'

    @patch.object(system, 'open', create=True)
    def test_is_openbmc(self, mocked_open):
        for (data, is_magic) in [
            (b'this is a unit test\n', False),
            (b'\\S\n', False),
            (b'OpenBMC Release wedge-v51\n', True),
            (b'Open BMC Release v25 \\n \\l\n', True),
        ]:
            mocked_open.return_value = mock_open(read_data=data).return_value
            self.assertEqual(system.is_openbmc(), is_magic)

    @patch.object(system, 'open', create=True)
    def test_get_mtds_single_full_mode(self, mocked_open):
        for name in ['flash0', 'flash', 'Partition_000']:
            data = textwrap.dedent('''\
            dev:    size   erasesize  name
            mtd0: 02000000 00010000 "{}"
            ''').format(name)
            mocked_open.return_value = mock_open(read_data=data).return_value
            for mtds in system.get_mtds():
                self.assertEqual(len(mtds), 1)
                self.assertEqual(mtds[0].file_name, '/dev/mtd0')
                self.assertEqual(mtds[0].size, 0x2000000)
                self.assertEqual(mtds[0].device_name, name)

    @patch.object(system, 'open', create=True)
    def test_get_mtds_dual_super_fit_mode(self, mocked_open):
        for order in [(0, 1), (1, 0)]:
            data = textwrap.dedent('''\
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
            ''').format(order[0], order[1])
            mocked_open.return_value = mock_open(read_data=data).return_value
            (full_mtds, all_mtds) = system.get_mtds()
            self.assertEqual(len(full_mtds), 2)
            self.assertEqual(full_mtds[0].device_name, 'flash1')
            self.assertEqual(len(all_mtds), 12)

    @patch.object(subprocess, 'check_output')
    def test_flash_too_big(self, mocked_check_output):
        self.mock_image.size = 2
        with self.assertRaises(SystemExit) as context_manager:
            system.flash(1, self.mock_image, self.mock_mtd, self.logger)
            mocked_check_output.assert_not_called()
            self.assertEqual(context_manager.exception.code, 1)

    @patch.object(system, 'open', create=True)
    @patch.object(system, 'glob', return_value=['/proc/123/cmdline',
                                                '/proc/self/cmdline'])
    @patch.object(os, 'getpid', return_value=123)
    def test_other_flasher_running_skip_self(
        self, mocked_getpid, mocked_glob, mocked_open
    ):
        self.assertEqual(system.other_flasher_running(self.logger), False)
        mocked_open.assert_not_called()

    @patch.object(system, 'open', create=True)
    @patch.object(system, 'glob')
    @patch.object(os, 'getpid', return_value=123)
    def test_other_flasher_running_return_false(
        self, mocked_getpid, mocked_glob, mocked_open
    ):
        read_data = [
            b'runsv\x00/etc/sv/restapi\x00',
            b'/usr/bin/mTerm_server\x00wedge\x00/dev/ttyS1\x00',
            b'/usr/sbin/ntpd\x00-p\x00/var/run/ntpd.pid\x00-g\x00',
            b'\x00',
        ]
        # Return the full list of /proc/$pid/cmdline from a single glob() call
        mocked_glob.return_value = [
            '/proc/{}/cmdline'.format(200 + i) for i in range(len(read_data))
        ]
        # Return just one cmdline from each open().read()
        mocked_open.side_effect = [
            mock_open(read_data=datum).return_value for datum in read_data
        ]
        self.assertEqual(system.other_flasher_running(self.logger), False)

    @patch.object(system, 'open', create=True)
    @patch.object(system, 'glob', return_value=['/proc/456/cmdline'])
    @patch.object(os, 'getpid', return_value=123)
    def test_other_flasher_running_return_true(
        self, mocked_getpid, mocked_glob, mocked_open
    ):
        read_data = [
            b'python\x00/usr/local/bin/psu-update-delta.py\x00',
            b'jbi\x00-i\x00/tmp/WEDGE100_SYSCPLD_v6_101.jbc\x00',
        ]
        for datum in read_data:
            mocked_open.return_value = mock_open(read_data=datum).return_value
            self.assertEqual(system.other_flasher_running(self.logger), True)

    @patch.object(system, 'other_flasher_running', return_value=True)
    @patch.object(subprocess, 'check_call')
    def test_flash_while_flashing_refused(
        self, mocked_check_call, mocked_other_flasher_running
    ):
        with self.assertRaises(SystemExit) as context_manager:
            system.flash(1, self.mock_image, self.mock_mtd, self.logger)
            mocked_check_call.assert_not_called()
            self.assertEqual(context_manager.exception.code, 1)

    @patch.object(system, 'other_flasher_running', return_value=False)
    def test_flash_retries(
        self, mocked_other_flasher_running
    ):
        for combination in range(100):
            attempts = combination % 10
            failures = combination // 10
            if attempts > failures + 1:
                continue
            # Create a new one every iteration, instead of reusing a single
            # patch decorator instance, to make debugging failed call count
            # assertions simple.
            with patch.object(subprocess, 'check_call') as mocked_check_call:
                return_values = [MagicMock()]
                flashcp = call(['flashcp', self.mock_image.file_name,
                                self.mock_mtd.file_name])
                calls = []
                for _ in range(min(attempts, failures)):
                    return_values.insert(
                        0, subprocess.CalledProcessError(255, 'in', 'out')
                    )
                    calls.append(flashcp)
                mocked_check_call.side_effect = return_values
                system.flash(attempts, self.mock_image, self.mock_mtd,
                             self.logger)
                self.assertNotEqual(mocked_other_flasher_running.call_count, 0)
                if attempts == 0:
                    self.assertEqual(mocked_check_call.call_count, 1)
                    self.assertNotEqual(mocked_check_call.call_args[0][0],
                                        'flashcp')
                else:
                    mocked_check_call.assert_has_calls(calls)

    @patch.object(system, 'other_flasher_running', return_value=True)
    @patch.object(subprocess, 'call')
    def test_reboot_while_flashing_refused(
        self, mocked_call, mocked_other_flasher_running
    ):
        with self.assertRaises(SystemExit) as context_manager:
            system.reboot(False, 'testing', self.logger)
            mocked_call.assert_not_called()
            self.assertEqual(context_manager.exception.code, 1)

    @patch.object(system, 'other_flasher_running', return_value=False)
    @patch.object(subprocess, 'call')
    @patch.object(logging, 'shutdown')
    def test_reboot(
        self, mocked_shutdown, mocked_call, mocked_other_flasher_running
    ):
        inputs_outputs = [
            (True, [
                'wall',
                'pypartition would be testing if this were not a dry run.'
            ]),
            (False, ['shutdown', '-r', 'now', 'pypartition is testing.']),
        ]
        for (dry_run, expected_arguments) in inputs_outputs:
            with self.assertRaises(SystemExit):
                system.reboot(dry_run, 'testing', self.logger)
                mocked_call.assert_called_with(expected_arguments)
                mocked_shutdown.assert_called()
