#!/usr/bin/env python2

#  Copyright (c) 2014-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.

import pexpect
import os
import sys
import argparse
import subprocess


QEMU_OPTS = "-nographic -M ast2500-edk -m 256M"
QEMU_DRIVE = "-drive if=mtd,format=raw,file="


def expected(buffer, matches):
    for match in matches:
        if buffer.find(match) < 0:
            raise Exception('Cannot find: %s \n%s' % (match, buffer))

def run(args):
    print "Using CMD %s" % (args)
    c = pexpect.spawn(args)
    c.logfile = sys.stdout
    return c

def expect_close(proc):
    try:
        proc.close(force=True)
    except Exception as e:
        print("Could not close process: %s" % (str(e)))

def expect_code(flash0, flash1, error_type, error_code):
    subprocess.call([
        'dd', 'if=/dev/zero', 'of=%s/flash.CS0.shell' % (OUTPUT),
        'bs=1k', 'count=%d' % (32 * 1024)
    ])
    subprocess.call([
        'dd', 'if=/dev/zero', 'of=%s/flash.CS1.shell' % (OUTPUT),
        'bs=1k', 'count=%d' % (32 * 1024)
    ])
    subprocess.call([
        'dd', 'if=%s/flashes/%s.CS0.%s' % (OUTPUT, FLASH_NAME, flash0),
        'of=%s/flash.CS0.shell' % (OUTPUT),
        'bs=1k', 'conv=notrunc'
    ])
    subprocess.call([
        'dd', 'if=%s/flashes/%s.CS1.%s' % (OUTPUT, FLASH_NAME, flash1),
        'of=%s/flash.CS1.shell' % (OUTPUT),
        'bs=1k', 'conv=notrunc'
    ])

    cmd = "%s %s %s%s %s%s" % (
        QEMU, QEMU_OPTS,
        QEMU_DRIVE, "%s/flash.CS0.shell" % (OUTPUT),
        QEMU_DRIVE, "%s/flash.CS1.shell" % (OUTPUT),
    )
    c = run(cmd)
    c.expect('Delete')
    c.send('\x1b\x5b\x33\x7e')
    c.expect('=> ')
    c.sendline('vbs')
    c.expect('\n=>', timeout=5)
    expected(c.before, [
        'Status: type (%d) code (%d)' % (error_type, error_code)
    ])
    return c

def test_success_state():
    c = expect_code('0.0', '0.0', 0, 0)
    expected(c.before, [
        'recovery_boot:     0'
    ])
    expect_close(c)

def test_bad_magic():
    c = expect_code('3.30', '3.30', 3, 30)
    expect_close(c)

def test_no_images():
    c = expect_code('3.31', '3.31', 3, 31)
    expect_close(c)

def test_no_firmware():
    c = expect_code('3.32', '3.32', 3, 32)
    expect_close(c)

def test_no_config():
    c = expect_code('3.33', '3.33', 3, 33)
    expect_close(c)

def test_no_keys():
    c = expect_code('3.35', '3.35', 3, 35)
    expect_close(c)

def test_bad_keys():
    c = expect_code('3.36', '3.36', 3, 36)
    expect_close(c)

def test_bad_firmware_missing_position():
    c = expect_code('3.37.1', '3.37.1', 3, 37)
    expect_close(c)

def test_bad_firmware_missing_size():
    c = expect_code('3.37.2', '3.37.2', 3, 37)
    expect_close(c)

def test_bad_firmware_huge_size():
    c = expect_code('3.37.3', '3.37.3', 3, 37)
    expect_close(c)

def test_invalid_size():
    c = expect_code('3.38', '3.38', 3, 38)
    expect_close(c)

def test_keys_invalid_different_kek():
    c = expect_code('4.40.1', '4.40.1', 4, 40)
    expect_close(c)

def test_keys_invalid_bad_hint():
    c = expect_code('4.40.2', '4.40.2', 4, 40)
    expect_close(c)

def test_keys_invalid_bad_data():
    c = expect_code('4.40.3', '4.40.3', 4, 40)
    expect_close(c)

def test_firmware_invalid_bad_timestamp():
    c = expect_code('4.42.1', '4.42.1', 4, 42)
    expect_close(c)

def test_firmware_invalid_bad_hash():
    c = expect_code('4.42.2', '4.42.2', 4, 42)
    expect_close(c)

def test_firmware_invalid_bad_hint():
    c = expect_code('4.42.3', '4.42.3', 4, 42)
    expect_close(c)

def test_firmware_invalid_duplicate_hash():
    c = expect_code('4.42.4', '4.42.4', 4, 42)
    expect_close(c)

def test_firmware_unverified():
    c = expect_code('4.43', '4.43', 4, 43)
    expect_close(c)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run verified-boot unit tests')
    parser.add_argument("qemu", metavar="QEMU",
        help="Location of QEMU binary")
    parser.add_argument("output", metavar="TEST-DIR",
        help="The output path used in create-tests")
    parser.add_argument("flash_name", metavar="FLASH-NAME",
        help="Name of the flash (flash-fbtp, flash0, etc) used in create-tests")
    args = parser.parse_args()

    QEMU = args.qemu
    OUTPUT = args.output
    FLASH_NAME = args.flash_name

    test_success_state()
    test_bad_magic()
    test_no_images()
    test_no_firmware()
    test_no_config()
    test_no_keys()
    test_bad_keys()
    test_bad_firmware_missing_position()
    test_bad_firmware_missing_size()
    test_bad_firmware_huge_size()
    test_invalid_size()
    test_keys_invalid_different_kek()
    test_keys_invalid_bad_hint()
    test_keys_invalid_bad_data()
    test_firmware_invalid_bad_timestamp()
    test_firmware_invalid_bad_hash()
    test_firmware_invalid_bad_hint()
    test_firmware_invalid_duplicate_hash()
    test_firmware_unverified()
