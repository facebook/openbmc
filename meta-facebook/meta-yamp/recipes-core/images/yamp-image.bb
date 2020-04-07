# Copyright 2018-present Facebook. All Rights Reserved.

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
  weutil-dhcp-id \
  fscd \
  led-controld \
  spatula \
  ncsi-util \
  "
