# Copyright 2022-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require recipes-core/images/fb-openbmc-image.bb
require fboss-lite-image-layout.inc

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  lldp-util \
  mterm \
  openbmc-utils \
  usb-console \
  wedge-eeprom \
  weutil-dhcp-id \
  ipmi-lite \
  kcsd \
  mdio-util \
  flashrom \
  "

# Add vboot-utils
IMAGE_INSTALL += '${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", "vboot-utils", "", d)}'
