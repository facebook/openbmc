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

FBOBMC_IMAGE_META_OFFSET = "0x003E8000"

def fbobmc_get_image_part_table(d):
    image_max_size = 0x08000000

    # The AST2700 boot firmware map reserves the first 4000 KiB for its
    # firmware bundle, so Facebook image metadata starts at 0x003e8000 and
    # occupies the remaining 96 KiB before the environment at 0x00400000.
    #
    # The SPI NOR has 64 KiB physical erase blocks, but 0x003e8000 is aligned
    # to only 32 KiB.  This value is therefore the image generator's layout
    # validation granularity, not the NOR erase size.  Linux exposes u-boot
    # and meta read-only because of that boundary; full-image updates use the
    # aligned flash0 partition instead of erasing either partition directly.
    erase_block_size = 32 * 1024

    part_table = (
        # offset,     name          max size (KiB)  type       source component
        (0x00000000, "u-boot",     -1,             "raw",     d.expand("${DEPLOY_DIR_IMAGE}/u-boot.${UBOOT_SUFFIX}")),
        (0x003E8000, "image-meta", 96,             "meta",    0),
        (0x00400000, "u-boot-env", 128,            "data",    0),
        (0x00420000, "os-fit",     -1,             "fit",     d.expand("${DEPLOY_DIR_IMAGE}/${FIT_LINK}")),
        (0x06000000, "data0",      32 * 1024,      "mtdonly", None),
    )

    return (image_max_size, erase_block_size, list(part_table))
