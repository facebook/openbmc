#!/usr/bin/env python

#  Copyright (c) 2014-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.

import pexpect
import os
import sys

QEMU_OPTS="-nographic -M ast2500-edk -m 256M"
QEMU_DRIVE="-drive if=mtd,format=raw,file="

def expected(buffer, matches):
    for match in matches:
        if buffer.find(match) < 0:
            raise Exception('Cannot find: %s \n%s' % (match, buffer))

def run(args):
    print "Using CMD %s" % (args)
    c = pexpect.spawn(args)
    c.logfile = sys.stdout
    return c

def test_success_state():
    cmd = "%s %s %s%s %s%s" % (
        QEMU, QEMU_OPTS,
        QEMU_DRIVE, "/opt/spl-automate/content/flash0",
        QEMU_DRIVE, "/opt/spl-automate/content/flash1.signed"
    )
    c = run(cmd)

    c.expect('\n=> ')
    c.sendline('vbs')
    c.expect('\n=>', timeout=5)
    expected(c.before, [
        'Status: type (0) code (0)'
    ])
    c.close()

def test_corrupted_uboot():
    cmd = "%s %s %s%s %s%s" % (
        QEMU, QEMU_OPTS,
        QEMU_DRIVE, "/opt/spl-automate/content/flash0",
        QEMU_DRIVE, "/opt/spl-automate/content/flash1.corrupted-uboot"
    )
    c = run(cmd)
    c.expect('\n=> ')
    c.sendline('vbs')
    c.expect('\n=> ')
    # Will cause execute failure (since booting is allowed to continue)
    expected(c.before, [
        'Status: type (2) code (11)',
        'Flags recovery_boot:     1',
        'Flags recovery_retries:  1',
    ])

def test_fake_subordinate():
    cmd = "%s %s %s%s %s%s" % (
        QEMU, QEMU_OPTS,
        QEMU_DRIVE, "/opt/spl-automate/content/flash0",
        QEMU_DRIVE, "/opt/spl-automate/content/flash1.fake-subordinate"
    )
    c = run(cmd)
    c.expect('\n=> ')
    c.sendline('vbs')
    c.expect('\n=> ')
    expected(c.before, [
        'Status: type (4) code (40)'
    ])

def test_fake_signature():
    cmd = "%s %s %s%s %s%s" % (
        QEMU, QEMU_OPTS,
        QEMU_DRIVE, "/opt/spl-automate/content/flash0",
        QEMU_DRIVE, "/opt/spl-automate/content/flash1.fake-signature"
    )
    c = run(cmd)
    c.expect('\n=> ')
    c.sendline('vbs')
    c.expect('\n=> ')
    expected(c.before, [
        'Status: type (4) code (42)'
    ])

def test_corrupted_fit():
    cmd = "%s %s %s%s %s%s" % (
        QEMU, QEMU_OPTS,
        QEMU_DRIVE, "/opt/spl-automate/content/flash0",
        QEMU_DRIVE, "/opt/spl-automate/content/flash1.corrupted-fit"
    )
    c = run(cmd)
    c.expect('\n=> ')
    c.sendline('vbs')
    c.expect('\n=> ')
    expected(c.before, [
        'Status: type (3) code (35)'
    ])

def test_missing_flash1():
    """In QEMU the second SPI chip will exists without content."""
    cmd = "%s %s %s%s" % (
        QEMU, QEMU_OPTS, QEMU_DRIVE, "/opt/spl-automate/content/flash0"
    )
    c = run(cmd)
    c.expect('\n=> ')
    c.sendline('vbs')
    c.expect('\n=> ')
    expected(c.before, [
        # Expect the VBS_ERROR_BAD_MAGIC error code.
        'Status: type (3) code (30)',
        'Flags recovery_retries:  1',
        'Flags recovery_boot:     1',
    ])
    c.close()

if __name__ == '__main__':
    QEMU = os.environ['QEMU']

    test_missing_flash1()
    test_corrupted_uboot()
    test_corrupted_fit()
    test_fake_subordinate()
    test_fake_signature()
    test_success_state()
