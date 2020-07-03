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
  bic-cache \
  bic-util \
  bitbang \
  cpldupdate \
  cpldupdate-i2c \
  crashdump \
  flashrom \
  front-paneld \
  fscd \
  fw-util \
  bic-monitor \
  ipmbd \
  ipmid \
  libcpldupdate-dll-echo \
  libcpldupdate-dll-gpio \
  lldp-util \
  log-util-v2 \
  mterm \
  openbmc-gpio \
  openbmc-utils \
  psu-util \
  sensor-setup \
  sensor-util \
  sensor-mon \
  spatula \
  threshold-util \
  usb-console \
  wedge-eeprom \
  weutil-dhcp-id \
  "
