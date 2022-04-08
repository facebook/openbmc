# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

IMAGE_FSTYPES_remove = "cpio.lzma.u-boot"
IMAGE_FSTYPES += "cpio.zst.u-boot"

require recipes-core/images/fb-openbmc-image.bb

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
  openbmc-gpio \
  rackmon \
  psu-update \
  sensor-setup \
  usb-console \
  lldp-util \
  wedge-eeprom \
  weutil-dhcp-id \
  kcsd \
  ipmid \
  po-eeprom \
  bitbang \
  jbi \
  flashrom \
  psumuxmon \
  mterm \
  fscd  \
  recover-from-secondary \
  udev-rules \
  "
