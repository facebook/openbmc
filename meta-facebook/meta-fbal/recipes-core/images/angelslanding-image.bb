# Copyright 2018-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require fbal-image-layout.inc
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
  plat-utils \
  fscd \
  fan-util \
  power-util \
  mterm \
  front-paneld \
  ipmid \
  fruid \
  sensor-util \
  sensor-mon \
  log-util-v2 \
  kcsd \
  healthd \
  fpc-util \
  fw-util \
  cfg-util \
  ipmi-util \
  peci-util-v2 \
  asd \
  asd-test \
  ipmitool \
  bios-util \
  vboot-utils \
  ncsi-util \
  ipmbd \
  ipmb-util \
  me-util \
  gpiod \
  guid-util \
  crashdump \
  threshold-util \
  cm-util \
  ncsid-v2 \
  name-util \
  mctp-util \
  "
