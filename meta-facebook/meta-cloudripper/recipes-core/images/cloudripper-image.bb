# Copyright 2020-present Facebook. All Rights Reserved.

#
# u-boot binary is ~500KB, so let's adjust u-boot max size and fit image
# location as below:
#   - u-boot partition: 0 - 0xDF000 (892KB, 913,408 bytes)
#   - u-boot-env partition: 0xE0000 - 0xFFFFF (128KB)
#   - FIT image starts at: 0x100000 (1MB)
#
UBOOT_PART_MAX_BYTES = "913408"
UBOOT_CKSUM_OFFSET_KB = "892"
FLASH_FIT_OFFSET_KB = "1024"

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  openbmc-utils \
  usb-console \
  "
