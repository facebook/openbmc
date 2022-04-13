# Copyright 2022-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require fbdarwin-image-layout.inc

require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  lldp-util \
  mterm \
  openbmc-utils \
  wedge-eeprom \
  weutil-dhcp-id \
  "

# Add vboot-utils for FBDARWINVBOOT image
IMAGE_INSTALL += '${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", "vboot-utils", "", d)}'
