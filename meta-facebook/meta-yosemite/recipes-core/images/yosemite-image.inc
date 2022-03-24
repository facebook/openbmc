# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

FLASH_ROOTFS_OFFSET = "4608"

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  healthd \
  fbutils \
  fan-util \
  fscd \
  sensor-setup \
  ipmid \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  fruid \
  ipmbd \
  bic-cached \
  bic-util \
  yosemite-sensors \
  sensor-util \
  sensor-mon \
  gpiod \
  gpiointrd \
  front-paneld \
  power-util \
  mterm\
  cfg-util \
  fw-util \
  fpc-util \
  me-util \
  log-util-v2 \
  lldp-util \
  openbmc-utils \
  conf-util \
  libncsi \
  ncsi-util \
  ncsid \
  obmc-libpldm \
  slot-util \
  snapshot-util \
  name-util \
  "
