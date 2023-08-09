# Copyright 2018-present Facebook. All Rights Reserved.

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
  at93cx6-util \
  bcm5396-util \
  ast-mdio \
  fan-ctrl \
  rackmon \
  psu-update \
  sensor-setup \
  usb-console \
  oob-nic \
  lldp-util \
  wedge-eeprom \
  weutil-dhcp-id \
  kcsd \
  ipmid \
  po-eeprom \
  bitbang \
  mterm \
  "
