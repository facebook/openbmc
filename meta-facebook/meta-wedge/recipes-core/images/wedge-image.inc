# Copyright 2018-present Facebook. All Rights Reserved.

inherit aspeed_uboot_image

require recipes-core/images/fb-openbmc-image.bb

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
IMAGE_FSTYPES += "cpio.lzma.u-boot"

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  openbmc-utils \
  at93cx6-util \
  bcm5396-util \
  ast-mdio \
  fan-ctrl \
  rackmon \
  watchdog-ctrl \
  sensor-setup \
  usb-console \
  oob-nic \
  lldp-util \
  wedge-eeprom \
  kcsd \
  ipmid \
  po-eeprom \
  bitbang \
  spatula \
  mterm \
  "
