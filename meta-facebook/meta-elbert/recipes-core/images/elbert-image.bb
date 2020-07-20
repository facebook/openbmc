# Copyright 2020-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  ast-mdio \
  flashrom \
  fscd \
  lldp-util \
  mterm \
  openbmc-gpio \
  openbmc-utils \
  spatula \
  usb-console \
  wedge-eeprom \
  weutil-dhcp-id \
  "
