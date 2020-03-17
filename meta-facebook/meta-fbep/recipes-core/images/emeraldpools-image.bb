# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

IMAGE_FSTYPES_remove = "cpio.lzma.u-boot"
IMAGE_FSTYPES += "cpio.zst.u-boot"

require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  packagegroup-openbmc-emmc \
  openbmc-utils \
  fscd \
  fan-util \
  power-util \
  ipmid \
  fruid \
  sensor-util \
  sensor-mon \
  log-util \
  healthd \
  fw-util \
  cfg-util \
  ipmi-util \
  ipmitool \
  vboot-utils \
  plat-utils \
  ipmbd \
  ipmb-util \
  gpiod \
  guid-util \
  mterm \
  front-paneld \
  fpc-util \
  tpm2-tools \
  threshold-util \
  "
