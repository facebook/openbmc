# Copyright 2018-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require fbcc-image-layout.inc

inherit kernel_fitimage

# TODO Disable ZSTD for now, enable once z-std patches are ported to Kernel 5.6
#IMAGE_FSTYPES_remove = "cpio.lzma.u-boot"
#IMAGE_FSTYPES += "cpio.zst.u-boot"

require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  openbmc-utils \
  log-util-v2 \
  fan-util \
  cfg-util \
  threshold-util \
  healthd \
  front-paneld \
  plat-utils \
  ipmid \
  ipmbd \
  ipmb-util\
  power-util \
  "
