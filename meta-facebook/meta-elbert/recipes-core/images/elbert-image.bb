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
  lldp-util \
  mterm \
  openbmc-gpio \
  openbmc-utils \
  usb-console \
  wedge-eeprom \
  "
# ELBERTTODO 447407 led-controld
# ELBERTTODO 442091 fscd using inlet
  # led-controld\
  # wedge-eeprom \
  # weutil-dhcp-id \
  # fscd \
  # spatula \
