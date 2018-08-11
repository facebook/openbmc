#!/usr/bin/env python2

#  Copyright (c) 2014-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.

import pexpect
import sys
import argparse
import subprocess

TPM_SOCKET = '/var/run/tpm/tpmd_socket:0'
QEMU_DRIVE = '-drive if=mtd,format=raw,file='
QEMU_OPTS = [
    '-nographic',
    '-M ast2500-edk',
    '-m 256M',
]


def expected(buffer, matches):
    for match in matches:
        if buffer.find(match) < 0:
            raise Exception('Cannot find: %s \n%s' % (match, buffer))


def expect_or_expected(c, match):
    try:
        expected(c.before, [match])
        return True
    except Exception:
        c.expect(match)


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


def create_call(flash0, flash1):
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
        QEMU, ' '.join(QEMU_OPTS),
        QEMU_DRIVE, "%s/flash.CS0.shell" % (OUTPUT),
        QEMU_DRIVE, "%s/flash.CS1.shell" % (OUTPUT),
    )
    return run(cmd)


def expect_code(flash0, flash1, error_type, error_code):
    c = create_call(flash0, flash1)
    c.expect('Delete')
    c.send('\x1b\x5b\x33\x7e')
    c.expect('=> ')
    c.sendline('vbs')
    c.expect('\n=>', timeout=5)
    expected(c.before, [
        'Status: type (%d) code (%d)' % (error_type, error_code)
    ])
    return c


def expect_kernel(
    flash0,
    flash1,
    error_type,
    error_code,
    location='280e0000',
    verify=True,
    failure=None
):
    c = create_call(flash0, flash1)
    if verify:
        c.expect('Verifying Hash Integrity')

    expect_or_expected(c, 'Loading kernel from FIT Image at %s' % (location))

    if error_type == 0:
        expect_or_expected(c, 'Starting kernel')
    elif failure is not None:
        c.expect(failure, timeout=5)
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
    # Returns general unverified firmware code.
    c = expect_code('3.36', '3.36', 4, 43)
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
    # Returns general unverified firmware code.
    c = expect_code('4.40.3', '4.40.3', 4, 43)
    expect_close(c)


def test_firmware_unverified():
    c = expect_code('4.43.1', '4.43.1', 4, 43)
    expect_close(c)


def test_firmware_invalid_bad_hint1():
    c = expect_code('4.43.2', '4.43.2', 4, 43)
    expect_close(c)


def test_firmware_invalid_bad_hint2():
    c = expect_code('4.43.3', '4.43.3', 4, 43)
    expect_close(c)


def test_firmware_invalid_duplicate_hash():
    c = expect_code('4.43.4', '4.43.4', 4, 43)
    expect_close(c)


def test_firmware_invalid_bad_timestamp():
    c = expect_code('4.43.5', '4.43.5', 4, 43)
    expect_close(c)


def test_firmware_invalid_bad_hash():
    c = expect_code('4.43.6', '4.43.6', 4, 43)
    expect_close(c)


def test_kernel():
    c = expect_kernel('0.0', '0.0', 0, 0)
    expect_close(c)


def test_kernel_fail():
    c = expect_kernel('0.0', '6.60.3', 6, 60, failure='Bad Data Hash')
    expect_close(c)


def test_kernel_corrupt():
    c = expect_kernel('0.0', '6.60.4', 6, 60, failure='Bad Data Hash')
    expect_close(c)


def test_kernel_early_fail():
    # This will not attempt to verify the kernel because vboot already failed.
    # The recovery kernel address (on SPI0 0x2000:0000) is expected.
    c = expect_kernel('0.0', '4.43.6', 0, 0, location='200e0000', verify=False)
    expect_close(c)


def test_fallback_last():
    c = expect_code('0.0', 'next.1', 0, 0)
    expect_close(c)
    c = expect_code('0.0', 'next.2', 0, 0)
    expect_close(c)
    c = expect_code('0.0', 'next.1', 0, 0)
    expect_close(c)


def test_fallback_fail():
    c = expect_code('0.0', '0.0', 9, 91)
    expect_close(c)


def test_fallback_forward_unsigned():
    # Try an unsigned image.
    c = expect_code('0.0', '4.43.5.1', 4, 43)
    expect_close(c)

    # It should not effect backups
    c = expect_code('0.0', 'next.1', 0, 0)
    expect_close(c)
    c = expect_code('0.0', 'next.2', 0, 0)
    expect_close(c)


def test_revoke_subordinate():
    c = expect_code('0.0', 'future', 0, 0)
    expect_close(c)

    # The subordinate changed
    c = expect_code('0.0', 'next.2', 9, 91)
    expect_close(c)
    c = expect_code('0.0', 'future', 0, 0)
    expect_close(c)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Run verified-boot unit tests')
    parser.add_argument(
        'qemu', metavar='QEMU',
        help='Location of QEMU binary')
    parser.add_argument(
        'output', metavar='TEST-DIR',
        help='The output path used in create-tests')
    parser.add_argument(
        'flash_name', metavar='FLASH-NAME',
        help='Name of the flash (flash-fbtp, flash0) used in create-tests')
    parser.add_argument(
        '--tpm', default=False, action='store_true',
        help='Also perform TPM tests',
    )
    args = parser.parse_args()

    QEMU = args.qemu
    OUTPUT = args.output
    FLASH_NAME = args.flash_name

    if args.tpm:
        QEMU_OPTS += [
            "-tpmdev passthrough,id=tpm0,cancel-path=/dev/null,path=%s" % (
                TPM_SOCKET),
            '-device tpm-tis,id=tpm0,tpmdev=tpm0,address=0x20',
        ]

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

    # 4.40 error code set.
    test_keys_invalid_different_kek()
    test_keys_invalid_bad_hint()
    test_keys_invalid_bad_data()

    # 4.43 error code set.
    test_firmware_unverified()
    test_firmware_invalid_bad_hint1()
    test_firmware_invalid_bad_hint2()
    test_firmware_invalid_duplicate_hash()
    test_firmware_invalid_bad_timestamp()
    test_firmware_invalid_bad_hash()

    # The OS/kernel tests.
    test_kernel()
    test_kernel_fail()
    test_kernel_corrupt()

    # This will force-recovery
    test_kernel_early_fail()

    # Fallback tests
    if args.tpm:
        test_fallback_last()
        test_fallback_fail()
        test_fallback_forward_unsigned()
        test_revoke_subordinate()
