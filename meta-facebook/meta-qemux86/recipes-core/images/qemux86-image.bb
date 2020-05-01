# Copyright 2018-present Facebook. All Rights Reserved.
#
IMAGE_FSTYPES_remove = "cpio.lzma.u-boot"
IMAGE_FSTYPES += "container"

inherit image_types

require recipes-core/images/fb-openbmc-image.bb

# The offset must match with the offsets defined in
# dev-spi-cmm.c. Rootfs starts from 4.5M
FLASH_ROOTFS_OFFSET = "4608"

# Include modules in rootfs
IMAGE_INSTALL += " \
  ptest-runner \
  "

# Certain image post-process commands tend to depend
# on the deploy directory being created by some process
# Since we do not use the standard image generation
# process, this tends to not be created. So, ensure
# this is created before the do_image_complete() executes.
IMAGE_PREPROCESS_COMMAND = " flash_image_generate ; "
flash_image_generate() {
  if [ ! -d ${DEPLOY_DIR_IMAGE} ]; then
    mkdir -p ${DEPLOY_DIR_IMAGE}
  fi
}
