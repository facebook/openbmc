# Copyright 2018-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require fbcc-image-layout.inc
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
  log-util-v2 \
  fan-util \
  cfg-util \
  threshold-util \
  healthd \
  front-paneld \
  plat-utils \
  ipmid \
  ipmi-util \
  ipmbd \
  ipmb-util\
  power-util \
  fruid \
  mac-util \
  ipmitool \
  gpiod \
  sensor-util \
  sensor-mon \
  vboot-utils \
  fscd \
  guid-util \
  fpc-util \
  mctp-util \
  name-util \
  "
