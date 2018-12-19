# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# The offset must match with the offsets defined in
# dev-spi-cmm.c. Rootfs starts from 4.5M
FLASH_ROOTFS_OFFSET = "4608"

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  openbmc-utils \
  at93cx6-util \
  ast-mdio \
  lldp-util \
  bitbang \
  flashrom \
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
  ipmbd \
  me-util \
  ipmb-util \
  log-util \
  kcsd \
  healthd \
  fpc-util \
  fw-util \
  cfg-util \
  ipmi-util \
  guid-util \
  gpiod \
  peci-util \
  asd \
  asd-test \
  ipmitool \
  bios-util \
  vboot-utils \
  crashdump \
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
