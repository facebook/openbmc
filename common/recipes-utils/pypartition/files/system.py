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

from virtualcat import ImageFile, MemoryTechnologyDevice, VirtualCat
from partition import (
    DeviceTreePartition,
    EnvironmentPartition,
    ExternalChecksumPartition,
    LegacyUBootPartition,
    Partition
)

import argparse
import json
import logging
import logging.handlers
import os
import re
import socket
import subprocess
import sys

from glob import glob

if False:
    from typing import List, Optional, Tuple, Union
    from virtualcat import ImageSourcesType
    LogHandlerType = Union[
        logging.StreamHandler,
        logging.FileHandler,
        logging.handlers.SysLogHandler
    ]
    LogDetailsType = List[Tuple[LogHandlerType, logging.Formatter]]
    MTDListType = List[MemoryTechnologyDevice]


def is_openbmc():
    # type: () -> bool
    if os.path.exists('/etc/issue'):
        magics = [b'Open BMC', b'OpenBMC']
        with open('/etc/issue', 'rb') as etc_issue:
            first_line = etc_issue.readline()
        return any(first_line.startswith(magic) for magic in magics)
    return False


standalone = logging.Formatter(
    '%(levelname)s:%(asctime)-15s %(message)s'
)


def get_logger():
    # type: () -> logging.Logger
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    loggers = [(logging.StreamHandler(), standalone)]  # type: LogDetailsType
    if is_openbmc():
        try:
            loggers.append((logging.FileHandler('/mnt/data/pypartition.log'),
                            standalone))
        except IOError:
            print('Error initializing log file in /mnt/data; using /tmp.',
                  file=sys.stderr)
            loggers.append((logging.FileHandler('/tmp/pypartition.log'),
                            standalone))
        try:
            loggers.append((logging.handlers.SysLogHandler('/dev/log'),
                            logging.Formatter('pypartition: %(message)s')))
        except socket.error:
            print('Error initializing syslog; skipping.', file=sys.stderr)
    for (handler, formatter) in loggers:
        handler.setFormatter(formatter)
        logger.addHandler(handler)
    return logger


def get_checksums_args(description):
    # type: (str) -> Tuple[List[str], argparse.Namespace]
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('image', nargs='?')
    checksum_help = 'currently required path to JSON file with dict mapping '
    checksum_help += 'md5sums to versions'
    parser.add_argument('--checksums', help=checksum_help, required=True,
                        type=argparse.FileType('r'))
    if is_openbmc():
        parser.add_argument('--dry-run', action='store_true')
    else:
        parser.add_argument('--serve', action='store_true')
        parser.add_argument('--port', type=int, default=2876)

    args = parser.parse_args()

    checksums = json.load(args.checksums).keys()

    return (checksums, args)


def free_kibibytes():
    # type: () -> int
    proc_meminfo_regex = re.compile(
        '^MemFree: +(?P<free_kb>[0-9]+) kB$', re.MULTILINE
    )
    with open('/proc/meminfo', 'r') as proc_meminfo:
        return int(proc_meminfo_regex.findall(proc_meminfo.read())[0])


def get_mtds():
    # type () -> Tuple[MTDListType, MTDListType]
    proc_mtd_regex = re.compile(
        '^(?P<dev>mtd[0-9]+): (?P<size>[0-9a-f]+) [0-9a-f]+ '
        '"(?P<name>[^"]+)"$', re.MULTILINE
    )
    mtd_info = []
    with open('/proc/mtd', 'r') as proc_mtd:
        mtd_info = proc_mtd_regex.findall(proc_mtd.read())
    vboot_support = 'none'
    if any(name == 'romx' for (_, _, name) in mtd_info):
        vboot_support = 'software-enforce'
        vboot_util_output = subprocess.check_output(
            ['/usr/local/bin/vboot-util']
        ).decode()
        if 'Flags hardware_enforce:  0x01' in vboot_util_output:
            vboot_support = 'hardware-enforce'
    all_mtds = []
    full_flash_mtds = []
    for (device, size_in_hex, name) in mtd_info:
        if name.startswith('flash') or name == 'Partition_000':
            if vboot_support == 'none' or \
               (vboot_support == 'software-enforce' and (name in ['flash0', 'flash1'])):
                    full_flash_mtds.append(MemoryTechnologyDevice(
                        device, int(size_in_hex, 16), name
                    ))
            elif vboot_support == 'hardware-enforce' and name in ['flash1']:
                    full_flash_mtds.append(MemoryTechnologyDevice(
                        device, int(size_in_hex, 16), name, 384
                    ))
        all_mtds.append(MemoryTechnologyDevice(
            device, int(size_in_hex, 16), name
        ))
    full_flash_mtds.sort(key=lambda mtd: mtd.device_name, reverse=True)
    return (full_flash_mtds, all_mtds)


def get_kernel_parameters():
    # type: () -> str
    # As far as cov knows, kernel parameters we use are backwards compatible,
    # so it's safe to use the following output even when there are CRC32
    # mismatch warnings.
    return subprocess.check_output(['fw_printenv', '-n', 'bootargs']).strip()


def append_to_kernel_parameters(dry_run, addition, logger):
    # type: (bool, str, logging.Logger) -> None
    before = get_kernel_parameters()
    logger.info('Kernel parameters before changes: {}'.format(before))
    if 'mtdparts' in before:
        logger.info('mtdparts already set in firmware environment.')
    if dry_run:
        logger.info('This is a dry run. Not changing kernel parameters.')
        return
    subprocess.call([
        'fw_setenv', 'bootargs', ' '.join([before, addition])
    ])
    logger.info('Kernel parameters after changes: {}'.format(
        get_kernel_parameters()
    ))


def get_partitions(images, checksums, logger):
    # type: (VirtualCat, List[str], logging.Logger) -> List[Partition]
    partitions = []  # type: List[Partition]
    next_magic = images.peek()
    # First 384K is u-boot for legacy or regular-fit images OR
    # the combination of SPL + recovery u-boot. Treat them as the same.
    if next_magic in ExternalChecksumPartition.UBootMagics:
        partitions.append(ExternalChecksumPartition(
            0x060000, 0x000000, 'u-boot', images, checksums, logger
        ))
    else:
        logger.error('Unrecognized magic 0x{:x} at offset 0x{:x}.'.format(
            next_magic, 0
        ))
        sys.exit(1)

    # Env is always in the same location for both legacy and FIT images.
    partitions.append(EnvironmentPartition(
        0x020000, 0x060000, 'env', images, logger
    ))

    # Either we are using the legacy image format or the FIT format.
    next_magic = images.peek()
    if next_magic == LegacyUBootPartition.magic:
        partitions.append(LegacyUBootPartition(
            [0x280000, 0x0400000], 0x080000, 'kernel', images, logger,
            LegacyUBootPartition.magic
        ))
        partitions.append(LegacyUBootPartition(
            [0xc00000, 0x1780000],
            partitions[-1].end(),
            'rootfs',
            images,
            logger,
        ))
    elif next_magic == DeviceTreePartition.magic:
        # The FIT image at 0x80000 could be a u-boot image (size 0x60000)
        # or the kernel+rootfs FIT which is much larger.
        # DeviceTreePartition() will pick the smallest which fits.
        part = DeviceTreePartition(
            [0x60000, 0x1B200000], 0x80000, "fit1", images, logger
        )
        partitions.append(part)

        # If the end of the above partition is 0xE0000 then we need to
        # check a second FIT image. This is definitely the larger one.
        if (part.end() == 0xE0000):
            partitions.append(DeviceTreePartition(
                [0x1B200000], 0xE0000, "fit2", images, logger
            ))
    else:
        logging.error('Unrecognized magic 0x{:x} at offset 0x{:x}.'.format(
            next_magic, 0x80000
        ))
        sys.exit(1)
    if images.images != []:
        # TODO data0 missing is only okay for ImageFiles, not
        # MemoryTechnologyDevices.  Also, this omits data0 from mtdparts=
        # message.
        partitions.append(Partition(
            0x2000000 - partitions[-1].end(),
            partitions[-1].end(),
            'data0',
            images,
            logger,
        ))
    return partitions


def get_valid_partitions(images_or_mtds, checksums, logger):
    # type: (ImageSourcesType, List[str], logging.Logger) -> List[Partition]
    if images_or_mtds == []:
        return []
    logger.info('Validating partitions in {}.'.format(
        ', '.join(map(str, images_or_mtds))
    ))
    with VirtualCat(images_or_mtds) as vc:
        partitions = get_partitions(vc, checksums, logger)
    last = None  # type: Optional[Partition]
    for current in partitions:
        # TODO populate valid env partition at build time
        # TODO learn to validate data0 partition
        if not current.valid and current.name not in ['env', 'data0']:
            message = 'Exiting due to invalid {} partition (details above).'
            logger.error(message.format(current))
            sys.exit(1)
        if last is not None and last.end() != current.partition_offset:
            logger.error('{} does not begin at 0x{:x}'.format(current,
                                                              last.end()))
            sys.exit(1)
        last = current
    return partitions


def run_verbosely(command, logger):
    # type: (List[str], logging.Logger) -> None
    command_string = ' '.join(command)
    logger.info('Starting to run `{}`.'.format(command_string))
    subprocess.check_call(command)
    logger.info('Finished running `{}`.'.format(command_string))

def other_flasher_running(logger):
    # type: (logging.Logger) -> bool
    basenames = [
        b'autodump.sh', b'cpld_upgrade.sh', b'dd', b'flash_eraseall', b'flashcp',
        b'flashrom', b'fw-util', b'fw_setenv', b'improve_system.py', b'jbi',
        b'psu-update-bel.py', b'psu-update-delta.py',
    ]
    running_flashers = {}

    our_cmdline = ['/proc/self/cmdline', '/proc/thread-self/cmdline',
        '/proc/{}/cmdline'.format(os.getpid())]

    for cmdline_file in glob('/proc/*/cmdline'):
        if cmdline_file in our_cmdline:
            continue
        try:
            with open(cmdline_file, 'rb') as cmdline:
                # Consider all command line parameters so `python foo.py` and
                # `foo.py` are both detected.
                for parameter in cmdline.read().split(b'\x00'):
                    basename = os.path.basename(parameter)
                    if basename in basenames:
                        if basename in running_flashers.keys():
                            running_flashers[basename] += 1
                        else:
                            running_flashers[basename] = 1
        # Processes and their respective files in procfs naturally come and go.
        except IOError:
            pass

    if running_flashers == {}:
        return False

    message = '{} running.'.format(b','.join(running_flashers.keys()).decode())
    logger.error(message)
    return True

def flash(attempts, image_file, mtd, logger):
    # type: (int, ImageFile, MemoryTechnologyDevice, logging.Logger) -> None
    if image_file.size > mtd.size:
        logger.error('{} is too big for {}.'.format(image_file, mtd))
        sys.exit(1)

    if other_flasher_running(logger):
        sys.exit(1)

    image_name = image_file.file_name
    # If MTD has a read-only offset, create a new image file with the
    # readonly offset from the device and the remaining from the image
    # file and use that for flashcp.
    if mtd.offset > 0:
        image_name = image_name + '.tmp'
        with open(image_name, 'wb') as out_f:
            with open(mtd.file_name, 'rb') as in_f:
                out_f.write(in_f.read(mtd.offset * 1024))
            with open(image_file.file_name, 'rb') as in_f:
                in_f.seek(mtd.offset * 1024)
                out_f.write(in_f.read())

    # TODO only write bytes that have changed
    flash_command = ['flashcp', image_name, mtd.file_name]
    if attempts < 1:
        flash_command = ['dd', 'if={}'.format(image_file.file_name),
                         'of=/dev/null']
        attempts = 1

    for attempt in range(attempts):
        try:
            run_verbosely(flash_command, logger)
            break
        except subprocess.CalledProcessError as result:
            # Retry the specified amount but even on consecutive failures don't
            # exit yet. Instead let the verification stage after this determine
            # the next steps.
            logger.warning('flashcp attempt {} returned {}.'.format(
                attempt, result.returncode
            ))
    # Remove temp file.
    if (image_file.file_name != image_name):
        os.remove(image_name)


# Unfortunately there is no mutual exclusion between flashing and rebooting.
# So for now minimize the window of opportunity for those operations to occur
# concurrently by rebooting as soon as possible after verification.
def reboot(dry_run, reason, logger):
    # type: (bool, str, logging.Logger) -> None

    if other_flasher_running(logger):
        sys.exit(1)

    reboot_command = ['shutdown', '-r', 'now',
                      'pypartition is {}.'.format(reason)]
    if dry_run:
        reboot_command = [
            'wall',
            'pypartition would be {} if this were not a dry run.'.format(
                reason
            )
        ]
        logger.info('This is a dry run. Not rebooting.')
    else:
        logger.info('Proceeding with reboot.')

    logging.shutdown()
    subprocess.call(reboot_command)
    # Trying to run anything after the `shutdown -r` command is issued would be
    # racing against shutdown killing this process.
    if subprocess.call(reboot_command) == 0:
        sys.exit(0)
    else:
        logger.error('Unable to reboot')
        sys.exit(1)
