# Copyright 2018-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require northdome-image-layout.inc

# IMAGE_FSTYPES_remove = "cpio.lzma.u-boot"
# IMAGE_FSTYPES += "cpio.zst.u-boot"

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
  watchdog-ctrl \
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
  ncsi-util \
  fby2-sensors \
  sensor-util \
  sensor-mon \
  gpiod \
  gpiointrd \
  front-paneld \
  power-util \
  mterm\
  cfg-util \
  fw-util \
  hsvc-util \
  fpc-util \
  crashdump \
  log-util-v2 \
  openbmc-utils \
  ipmi-util \
  guid-util \
  asd \
  asd-test \
  bios-util \
  threshold-util \
  ncsid-v2 \
  vboot-utils \
  slot-util \
  snapshot-util \
  apml-util \
  psb-util \
  setup-gpio \
  name-util \
  "

PROVIDES+="northdome-vboot-image"
