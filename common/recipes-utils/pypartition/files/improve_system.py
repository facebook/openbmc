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
from __future__ import absolute_import, division, print_function, unicode_literals

import os
import signal
import sys
import textwrap

import system
import virtualcat


# Python2 compatibility layer
try:
    FileNotFoundError
except NameError:
    FileNotFoundError = OSError

# For wedge100, there was a bug that caused the CS1 pin to be configured
# for GPIO instead. Manually configure it for Chip Select before attempting
# to write to flash1.
def fix_wedge100_romcs1():
    has_gpio_util = os.path.exists("/usr/local/bin/openbmc_gpio_util.py")
    if system.is_wedge100() and has_gpio_util:
        cmd = ["/usr/local/bin/openbmc_gpio_util.py", "config", "ROMCS1#"]
        system.run_verbosely(cmd, logger)


def free_mem_remediation(logger):
    # purge the logs
    system.flush_tmpfs_logs(logger)
    # restart services that are known to consume less memory in early phases
    system.restart_services(logger)
    # drop caches
    system.drop_caches(logger)

    return


def get_downloaded_image_size(logger, image):
    if not image:
        return 0

    # worst case scenario already accounted in calculations
    if image.startswith("http:") or image.startswith("https:"):
        return 0

    try:
        img_stat = os.stat(image)
        img_size_kb = int(img_stat.st_size / 1024)
        logger.info("Image size {} Kb".format(img_size_kb))
        return img_size_kb
    except FileNotFoundError:
        # this is weird, but let's leave it to "normal" pypartition flow
        logger.error(
            "File {} not found, can not calculate real minimum memory".format(image)
        )
        return 0


def improve_system(logger):
    # type: (object) -> None
    if not system.is_openbmc():
        logger.error("{} must be run from an OpenBMC system.".format(sys.argv[0]))
        sys.exit(1)

    description = textwrap.dedent(
        """\
        Leave the OpenBMC system better than we found it, by performing one or
        more of the following actions: fetching an image file, validating the
        checksums of the partitions inside an image file, copying the image
        file to flash, validating the checksums of the partitions on flash,
        changing kernel parameters, rebooting.

        The current logic is reboot-happy. If you want to validate the
        checksums of the partitions on flash without rebooting, use
        --dry-run."""
    )
    (checksums, args) = system.get_checksums_args(description)

    # Don't let things like dropped SSH connections interrupt.
    [
        signal.signal(s, signal.SIG_IGN)
        for s in [signal.SIGHUP, signal.SIGINT, signal.SIGTERM]
    ]

    (full_flash_mtds, all_mtds) = system.get_mtds()

    [total_memory_kb, free_memory_kb] = system.get_mem_info()
    logger.info(
        "{} KiB total memory, {} KiB free memory.".format(
            total_memory_kb, free_memory_kb
        )
    )

    # Execute light weight free memory remediation to reduce reboot remediation
    free_mem_remediation(logger)

    [total_memory_kb, free_memory_kb] = system.get_mem_info()
    logger.info(
        "After free memory remediation: {} KiB total memory, {} KiB free memory.".format(
            total_memory_kb, free_memory_kb
        )
    )
    reboot_threshold_pct = system.get_healthd_reboot_threshold()
    reboot_threshold_kb = ((100 - reboot_threshold_pct) / 100) * total_memory_kb

    # As of May 2019, sample image files are 17 to 23 MiB. Make sure
    # there is space for them and a few flashing related processes.
    max_openbmc_img_size = 23
    openbmc_img_size_kb = max_openbmc_img_size * 1024
    # in the case of no reboot threshold, use a default limit
    default_threshold = 60 * 1024

    # for low memory remediation - ensure downloading BMC image will NOT trigger
    # healthd reboot
    min_memory_needed = max(
        default_threshold, openbmc_img_size_kb + reboot_threshold_kb
    )
    logger.info(
        "Healthd reboot threshold at {}%  ({} KiB)".format(
            reboot_threshold_pct, reboot_threshold_kb
        )
    )

    # we need to take into account a case when image is already downloaded and
    # subtract its size from `min_memory_needed`
    min_memory_needed -= get_downloaded_image_size(logger, args.image)

    logger.info("Minimum memory needed for update is {} KiB".format(min_memory_needed))
    if free_memory_kb < min_memory_needed:
        logger.info(
            "Free memory ({} KiB) < minimum required memory ({} KiB), reboot needed".format(
                free_memory_kb, min_memory_needed
            )
        )
        if full_flash_mtds != []:
            [
                system.get_valid_partitions([full_flash], checksums, logger)
                for full_flash in full_flash_mtds
            ]
        else:
            system.get_valid_partitions(all_mtds, checksums, logger)
        system.reboot(args.dry_run, "remediating low free memory", logger)

    if full_flash_mtds == []:
        partitions = system.get_valid_partitions(all_mtds, checksums, logger)
        mtdparts = "mtdparts={}:{},-@0(flash0)".format(
            "spi0.0", ",".join(map(str, partitions))
        )
        system.append_to_kernel_parameters(args.dry_run, mtdparts, logger)
        system.reboot(args.dry_run, "changing kernel parameters", logger)

    reason = "potentially applying a latent update"
    if args.image:
        image_file_name = args.image
        if args.image.startswith("https"):
            try:
                cmd = ["curl", "-k", "-s", "-o", "/tmp/flash", args.image]
                system.run_verbosely(cmd, logger)
                image_file_name = "/tmp/flash"
            except Exception:
                # Python2 may throw OSError here, whereas Python3 may
                # throw FileNotFoundError - common case is Exception.
                args.image = args.image.replace("https", "http")

        if args.image.startswith("http:"):
            cmd = ["wget", "-q", "-O", "/tmp/flash", args.image]
            system.run_verbosely(cmd, logger)
            # --continue might be cool but it doesn't seem to work.
            image_file_name = "/tmp/flash"

        image_file = virtualcat.ImageFile(image_file_name)
        system.get_valid_partitions([image_file], checksums, logger)

        # If /mnt/data is smaller in the new image, it must be made read-only
        # to prevent the tail of the FIT image from being corrupted.
        # Determining shrinkage isn't trivial (data0 is omitted from image
        # image files for one thing) and details may change over time, so just
        # remount every MTD read-only. Reboot is expected to restart the
        # killed processes.
        system.fuser_k_mount_ro(system.get_writeable_mounted_mtds(), logger)

        # Don't let healthd reboot mid-flash (S166329).
        system.remove_healthd_reboot(logger)

        fix_wedge100_romcs1()

        attempts = 0 if args.dry_run else 3

        for mtd in full_flash_mtds:
            system.flash(attempts, image_file, mtd, logger, args.flash_name, args.force)
        # One could in theory pre-emptively set mtdparts for images that
        # will need it, but the mtdparts generator hasn't been tested on dual
        # flash and potentially other systems. To avoid over-optimizing for
        # what should be a temporary condition, just reboot and reuse the
        # existing no full flash logic.
        reason = "applying an update"

    [
        system.get_valid_partitions([full_flash], checksums, logger)
        for full_flash in full_flash_mtds
    ]
    system.reboot(args.dry_run, reason, logger)


if __name__ == "__main__":
    logger = system.get_logger()
    try:
        improve_system(logger)
    except Exception:
        logger.exception("Unhandled exception raised.")
        sys.exit(1)
