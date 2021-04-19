# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# The offset must match with the offsets defined in
# dev-spi-cmm.c. Rootfs starts from 4.5M
FLASH_ROOTFS_OFFSET = "4608"

# Include modules in rootfs
IMAGE_INSTALL += " \
  healthd \
  plat-utils \
  fan-util \
  fscd \
  sensor-setup \
  ipmid \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  fruid \
  ipmbd \
  bic-cached \
  bic-util \
  minilaketb-sensors \
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
  ipmi-util \
  guid-util \
  threshold-util \
  "
