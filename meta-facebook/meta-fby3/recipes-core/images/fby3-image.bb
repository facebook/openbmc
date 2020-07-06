# Copyright 2018-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc

IMAGE_FSTYPES_remove = "cpio.lzma.u-boot"
IMAGE_FSTYPES += "cpio.zst.u-boot"

require recipes-core/images/fb-openbmc-image.bb

# sensor-setup
# hsvc-util
# fpc-util
# imc-util
# sensordump
# enclosure-util
# nvme-util

# Include modules in rootfs
IMAGE_INSTALL += " \
  healthd \
  fan-util \
  fscd \
  ipmbd \
  ipmid \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  fruid \
  ncsi-util \
  sensor-util \
  sensor-mon \
  power-util \
  mterm\
  cfg-util \
  fw-util \
  log-util-v2 \
  lldp-util \
  spatula \
  openbmc-utils \
  ipmi-util \
  asd \
  asd-test \
  bios-util \
  threshold-util \
  libncsi \
  vboot-utils \
  libpldm \
  plat-utils \
  i2craw \
  bic-util \
  me-util \
  bic-cached \
  guid-util \
  gpiod \
  front-paneld \
  gpiointrd \
  crashdump \
  show-sys-config \
  slot-util \
  enclosure-util \
  fpc-util \
  at \
  ncsid-v2 \
  throttle-util \
  "
