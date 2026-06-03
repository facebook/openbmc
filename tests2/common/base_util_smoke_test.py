#!/usr/bin/env python3
#
# Copyright 2026-present Facebook. All Rights Reserved.
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

import subprocess

from utils.cit_logger import Logger


class BaseUtilSmokeTest(object):
    """QEMU-safe smoke test for platform utility binaries.

    The functional *_util tests (dimm-util, bios-util, ...) exercise their
    binaries with hardware-dependent arguments (FRUs, DIMMs, slots) and are
    skipped under QEMU -- either by an explicit @skipIf(qemu_check()) or by a
    runtime FRU/slot availability check. As a result a build that shipped a
    missing or unlinkable utility would pass the QEMU conveyor completely
    untested.

    This test closes that gap with two checks that need no hardware:
      1. `command -v <util>` -- the binary is installed and on PATH.
      2. `ldd <path>`        -- its shared library dependencies all resolve
                                (catches missing .so / loader errors).

    It is intentionally NOT decorated with @skipIf(qemu_check()): it must run
    in QEMU, and it is harmless on hardware.

    Platform subclasses mix this in with unittest.TestCase and set UTILS to the
    list of utility binary names expected on that platform, e.g.:

        class UtilSmokeTest(BaseUtilSmokeTest, unittest.TestCase):
            UTILS = ["dimm-util", "bios-util", ...]
    """

    # Override in the platform subclass with the utilities to check.
    UTILS = []

    def setUp(self):
        Logger.start(name=self._testMethodName)

    def resolve_path(self, util):
        """Return the resolved path of util, failing if it is not installed."""
        proc = subprocess.run(
            ["/bin/sh", "-c", 'command -v -- "$1"', "sh", util],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=10,
        )
        path = proc.stdout.decode("utf-8", "replace").strip()
        self.assertEqual(
            proc.returncode,
            0,
            "{} is not installed (command -v exited {})".format(util, proc.returncode),
        )
        self.assertTrue(path, "{}: command -v returned an empty path".format(util))
        return path

    def assert_libraries_resolve(self, util, path):
        """Fail if any shared library dependency of the binary is unresolved."""
        try:
            proc = subprocess.run(
                ["ldd", path],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=10,
            )
        except FileNotFoundError:
            # ldd not present on the image: fall back to presence-only (the
            # binary was already resolved by command -v). A missing ldd is a
            # tooling gap, not a regression in the util under test.
            Logger.info("ldd not available; skipping link check for {}".format(util))
            return
        out = proc.stdout.decode("utf-8", "replace")
        # Statically linked binaries have nothing to resolve; ldd says so and
        # may exit non-zero. Treat that as a pass.
        if "not a dynamic executable" in out:
            return
        missing = [line.strip() for line in out.splitlines() if "not found" in line]
        self.assertEqual(
            missing,
            [],
            "{} has unresolved shared libraries:\n{}".format(util, "\n".join(missing)),
        )

    def test_utils_installed_and_linked(self):
        self.assertTrue(self.UTILS, "no UTILS declared for this platform smoke test")
        for util in self.UTILS:
            with self.subTest(util=util):
                path = self.resolve_path(util)
                self.assert_libraries_resolve(util, path)
