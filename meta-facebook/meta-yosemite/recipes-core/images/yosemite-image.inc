# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
IMAGE_FSTYPES += "cpio.lzma.u-boot"
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
  spatula \
  openbmc-utils \
  conf-util \
  libncsi \
  ncsi-util \
  ncsid \
  libpldm \
  slot-util \
  "

IMAGE_FEATURES += " \
  ssh-server-openssh \
  tools-debug \
  "

DISTRO_FEATURES += " \
  ext2 \
  ipv6 \
  nfs \
  usbgadget \
  usbhost \
  "
