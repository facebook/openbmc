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
        logging.root.setLevel(logging.DEBUG)
        self.mock_image = MagicMock()
        self.mock_image.size = 1
        self.mock_mtd = MagicMock()
        self.mock_mtd.size = 1

    def test_get_mtds_single_full_mode(self):
        # Partition_000 (the default name that the Linux kernel generates if
        # no name is specified, for example if "mtdparts=spi0.0:-" is used) is
        # not currently supported.
        for name in ['flash0', 'flash']:
            data = textwrap.dedent('''\
            dev:    size   erasesize  name
            mtd0: 02000000 00010000 "{}"
            ''').format(name)
            with patch.object(
                system, "open", mock_open(read_data=data), create=True
            ):
                for mtds in system.get_mtds():
                    self.assertEqual(len(mtds), 1)
                    self.assertEqual(mtds[0].file_name, '/dev/mtd0')
                    self.assertEqual(mtds[0].size, 0x2000000)
                    self.assertEqual(mtds[0].device_name, name)

    def test_get_mtds_dual_super_fit_mode(self):
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
            with patch.object(
                system, "open", mock_open(read_data=data), create=True
            ):
                (full_mtds, all_mtds) = system.get_mtds()
                self.assertEqual(len(full_mtds), 2)
                self.assertEqual(full_mtds[0].device_name, 'flash1')
                self.assertEqual(len(all_mtds), 12)

    def test_flash_too_big(self):
        self.mock_image.size = 2
        with patch.object(subprocess, 'check_output') as mock_check_output:
            with self.assertRaises(SystemExit) as context_manager:
                system.flash(1, self.mock_image, self.mock_mtd, MagicMock())
                mock_check_output.assert_not_called()
                self.assertEqual(context_manager.exception.code, 1)

    def test_flash_competition(self):
        with patch.object(subprocess, 'check_output') as mock_check_output:
            mock_check_output.side_effect = '1234567'
            with self.assertRaises(SystemExit) as context_manager:
                system.flash(1, self.mock_image, self.mock_mtd, MagicMock())
                mock_check_output.assert_called_once()
                self.assertEqual(context_manager.exception.code, 1)

    def test_flash_retries(self):
        self.mock_image.file_name = '/tmp/myimagefile'
        self.mock_mtd.file_name = '/dev/mtd99'
        with patch.object(subprocess, 'check_output') as mock_check_output:
            mock_check_output.side_effect = subprocess.CalledProcessError(
                321, 'command', 'output'
            )
            for combination in range(100):
                attempts = combination % 10
                failures = combination // 10
                if attempts > failures + 1:
                    continue
                with patch.object(subprocess, 'check_call') as mock_check_call:
                    return_values = [MagicMock()]
                    flashcp = call(['flashcp', self.mock_image.file_name,
                                    self.mock_mtd.file_name])
                    calls = []
                    for _ in range(min(attempts, failures)):
                        return_values.insert(
                            0, subprocess.CalledProcessError(255, 'in', 'out')
                        )
                        calls.append(flashcp)
                    mock_check_call.side_effect = return_values
                    system.flash(attempts, self.mock_image, self.mock_mtd,
                                 MagicMock())
                    mock_check_output.assert_called_once()
                    if attempts == 0:
                        mock_check_call.assert_called_once()
                        self.assertNotEqual(mock_check_call.call_args[0][0],
                                            'flashcp')
                    else:
                        mock_check_call.assert_has_calls(calls)

    def test_reboot(self):
        inputs_outputs = [
            (True, [
                'wall',
                'pypartition would be testing if this were not a dry run.'
            ]),
            (False, ['shutdown', '-r', 'now', 'pypartition is testing.']),
        ]
        for (dry_run, expected_arguments) in inputs_outputs:
            with patch.object(subprocess, 'call') as mock_call:
                with patch.object(logging, 'shutdown') as mock_shutdown:
                    system.reboot(dry_run, 'testing', MagicMock())
                    mock_call.assert_called_with(expected_arguments)
                    mock_shutdown.assert_called()
