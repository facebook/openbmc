# Copyright 2018-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require fbep-image-layout.inc
require recipes-core/images/fb-zstd-rootfs.inc

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  openbmc-utils \
  fscd \
  fan-util \
  power-util \
  ipmid \
  fruid \
  sensor-util \
  sensor-mon \
  log-util-v2 \
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
  threshold-util \
  asic-util \
  mac-util \
  name-util \
  "
