#!/usr/bin/env python
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
import signal
import sys
import system
import textwrap
import virtualcat


def improve_system(logger):
    # type: (logging.Logger) -> None
    if not system.is_openbmc():
        logger.error('{} must be run from an OpenBMC system.'.format(
            sys.argv[0]
        ))
        sys.exit(1)

    description = textwrap.dedent('''\
        Leave the OpenBMC system better than we found it, by performing one or
        more of the following actions: fetching an image file, validating the
        checksums of the partitions inside an image file, copying the image
        file to flash, validating the checksums of the partitions on flash,
        changing kernel parameters, rebooting.

        The current logic is reboot-happy. If you want to validate the
        checksums of the partitions on flash without rebooting, use
        --dry-run.''')
    (checksums, args) = system.get_checksums_args(description)

    # Don't let things like dropped SSH connections interrupt.
    [signal.signal(s, signal.SIG_IGN) for s in
     [signal.SIGHUP, signal.SIGINT, signal.SIGTERM]]

    (full_flash_mtds, all_mtds) = system.get_mtds()

    free_memory = system.free_kibibytes()
    logger.info('{} KiB free memory.'.format(free_memory))
    # As of August 2017, sample image files are 15 to 22 MiB. Make sure there
    # is space for them and a few flashing related processes.
    if system.free_kibibytes() < 60 * 1024:
        if full_flash_mtds != []:
            [system.get_valid_partitions([full_flash], checksums, logger)
             for full_flash in full_flash_mtds]
        else:
            system.get_valid_partitions(all_mtds, checksums, logger)
        system.reboot(args.dry_run, 'remediating low free memory', logger)

    if full_flash_mtds == []:
        partitions = system.get_valid_partitions(all_mtds, checksums, logger)
        mtdparts = 'mtdparts={}:{},-@0(flash0)'.format(
            'spi0.0', ','.join(map(str, partitions))
        )
        system.append_to_kernel_parameters(args.dry_run, mtdparts, logger)
        system.reboot(args.dry_run, 'changing kernel parameters', logger)

    reason = 'potentially applying a latent update'
    if args.image:
        image_file_name = args.image
        if args.image.startswith('http'):
            # --continue might be cool but it doesn't seem to work.
            system.run_verbosely(
                ['wget', '-q', '-O', '/tmp/flash', args.image], logger
            )
            image_file_name = '/tmp/flash'

        image_file = virtualcat.ImageFile(image_file_name)
        system.get_valid_partitions([image_file], checksums, logger)

        attempts = 0 if args.dry_run else 3
        for mtd in full_flash_mtds:
            system.flash(attempts, image_file, mtd, logger)
        # One could in theory pre-emptively set mtdparts for images that
        # will need it, but the mtdparts generator hasn't been tested on dual
        # flash and potentially other systems. To avoid over-optimizing for
        # what should be a temporary condition, just reboot and reuse the
        # existing no full flash logic.
        reason = 'applying an update'

    [system.get_valid_partitions([full_flash], checksums, logger)
     for full_flash in full_flash_mtds]
    system.reboot(args.dry_run, reason, logger)


if __name__ == '__main__':
    logger = system.get_logger()
    try:
        improve_system(logger)
    except Exception:
        logger.exception('Unhandled exception raised.')
