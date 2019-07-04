# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  at93cx6-util \
  ast-mdio \
  bcm5396-util \
  bitbang \
  cpldupdate \
  fan-ctrl \
  libcpldupdate-dll-echo \
  libcpldupdate-dll-gpio \
  lldp-util \
  openbmc-utils \
  wedge-eeprom \
  weutil-dhcp-id \
  "
