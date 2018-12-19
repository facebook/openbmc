# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# The offset must match with the offsets defined in
# dev-spi-cmm.c. Rootfs starts from 4.5M
FLASH_ROOTFS_OFFSET = "4608"

# Include modules in rootfs
IMAGE_INSTALL += " \
  bic-cached \
  bic-util \
  bios-util \
  cfg-util \
  enclosure-util \
  expander-util \
  exp-cached \
  fan-util \
  fbutils \
  fpc-util \
  fruid \
  front-paneld \
  fscd \
  fw-util \
  gpiod \
  healthd \
  ipmbd \
  ipmid \
  ipmi-util \
  libexp \
  libmctp \
  lldp-util \
  log-util \
  me-util \
  mterm \
  openbmc-utils \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  power-util \
  sensor-mon \
  sensor-setup \
  sensor-util \
  spatula \
  usb-console \
  vboot-utils \
  libncsi \
  ncsi-util \
  ncsid \
  libpldm \
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
